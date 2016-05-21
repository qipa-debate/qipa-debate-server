#include <sys/time.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <signal.h>
#include <cstring>
#include <cinttypes>

#ifdef GOOGLE_PROFILE_FLAG
#include <gperftools/profiler.h>
#endif

#include "base/log.h"
#include "base/packer.h"
#include "base/process.h"
#include "base/safe_log.h"
#include "error_code.h"
#include "protocol/app_comm.h"
#include "recorder/audio_event_handler.h"

using namespace std;

extern bool g_record_pcm;
extern std::string g_record_directory;

namespace agora {
using namespace pstn;
using namespace base;
using namespace rtc;

// A frame of 10 ms
static const int kFrameInterval = 100;

namespace recorder {

// NOTE(liuyong): Improves me
enum PhoneEvent {UNKNOWN = 0, RING = 1, PICK_UP, HANG_UP};

atomic<bool> audio_event_handler::s_broken_pipe_(false);

inline int64_t now_us() {
  timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000ll * 1000 + now.tv_usec;
}

inline int32_t now_ts() {
  return static_cast<int32_t>(now_us() / 1000llu / 1000);
}

audio_event_handler::audio_event_handler(int read_fd, int write_fd, uint32_t uid,
    const string &vendor_key, const string &channel_name, bool mixed,
    int32_t samples, int32_t idle)
:read_fd_(read_fd), write_fd_(write_fd), uid_(uid), vendor_key_(vendor_key),
    channel_name_(channel_name), audio_mixed_(mixed),
    sample_rates_(samples), channel_idle_(idle) {
  reader_ = NULL;
  writer_ = NULL;
  applite_ = NULL;

  if (read_fd > 0) {
    reader_ = new (std::nothrow)pipe_reader(read_fd);
  }

  if (write_fd > 0) {
    writer_ = new (std::nothrow)pipe_writer(write_fd);
  }

  joined_ = false;
}

audio_event_handler::~audio_event_handler() {
  if (applite_) {
    cleanup();
  }
}

#ifdef GOOGLE_PROFILE_FLAG
struct profiler_guard {
  explicit profiler_guard(const char *file) {
    LOG(INFO, "Starting profiling...");
    ProfilerStart(file);
  }

  ~profiler_guard() {
    LOG(INFO, "Stopping profiling....");
    ProfilerStop();
  }
};
#endif

void audio_event_handler::cleanup() {
  if (applite_) {
    applite_->leaveChannel();

    // FIXME(liuyong): work around the crash in Trace thread
    sleep(1);
    applite_->release();
    applite_ = NULL;
  }

  delete reader_;
  delete writer_;
}

int audio_event_handler::run() {
#ifdef GOOGLE_PROFILE_FLAG
  profiler_guard guard("./profile");
#endif

  applite_ = dynamic_cast<rtc::IRtcEngineEx*>(createAgoraRtcEngine());
  if (applite_ == NULL) {
    SAFE_LOG(FATAL) << "Failed to create an Agora Rtc Engine!";
    return -1;
  }

  RtcEngineContextEx context;
  context.eventHandler = this;
  context.isExHandler = true;
  context.vendorKey = NULL;
  context.context = NULL;

  applite_->initializeEx(context);
  applite_->setLogCallback(true);

  last_active_ts_ = now_ts();

  // setup signal handler
  signal(SIGPIPE, broken_pipe_handler);
  signal(SIGINT, broken_pipe_handler);

  int64_t join_start_ts = now_us();
  // char sessionId[128];

  if (applite_->joinChannel(vendor_key_.c_str(), channel_name_.c_str(),
      NULL, uid_) < 0) {
    SAFE_LOG(ERROR) << "Failed to create the channel " << channel_name_;
    return -2;
  }

  int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timer_fd < 0) {
    LOG(ERROR, "Failed to create a timer fd! %m");
    applite_->leaveChannel();
    return -3;
  }

  int interval = kFrameInterval;
  int interval_ms = interval * 1000llu * 1000;
  itimerspec timer = {{0, interval_ms}, {0, interval_ms}};
  timerfd_settime(timer_fd, 0, &timer, NULL);

  errno = 0;

  pollfd fds[] = {{timer_fd, POLLIN, 0}, {read_fd_, POLLIN, 0}};
  int events = read_fd_ >= 0 ? 2 : 1;

  int ret_code = 0;
  int32_t last_check_ts = now_ts();
  int cnt = -1;

  while ((cnt = poll(fds, events, -1)) >= 0 || (cnt == -1 && errno == EINTR)) {
    if (s_broken_pipe_.load())
      break;

    // breaked by interrupts
    if (cnt == -1) {
      errno = 0;
      continue;
    }

    // check if it is timeout event
    if (fds[0].revents & POLLIN) {
      fds[0].revents = 0;

      static const int64_t MAX_TIMEOUT = 10 * 1000 * 1000ll;
      static const int32_t ACTIVE_CHECK_TS = 10;
      uint64_t expirations = 0;
      ssize_t readed = read(timer_fd, &expirations, sizeof(expirations));
      LOG_IF(ERROR, readed != sizeof(expirations), "Partial read!");
      assert(readed == sizeof(expirations));

      if (!joined_) {
        if (now_us() - join_start_ts >= MAX_TIMEOUT) {
          send_error_report(ERROR_JOIN_VOIP_TIMEOUT, "Join voip server timeout");
          LOG(WARN, "Max_Timeout elapsed before joinChannel returns");
          break;
        }
      } else {
        // Check if it is a solo channel
        int32_t now = now_ts();
        if (last_check_ts + ACTIVE_CHECK_TS < now) {
          last_check_ts = now;
          if (is_solo_channel()) {
            LOG(WARN, "The channel is idle over %d seconds.", channel_idle_);
            ret_code = -5;
            break;
          }
        }
      }
    }

    if (fds[1].revents & POLLIN) {
      fds[1].revents = 0;

      base::unpacker pk;
      uint16_t uri;
      base::ReadState state;
      while ((state = reader_->read_packet(&pk, &uri)) == base::kReceived) {
        handle_request(pk, uri);
      }

      if (state == base::kEof || state == base::kError) {
        LOG(INFO, "closed by pstn side: %d", state);
        ret_code = -1;
        break;
      }
    } else if (fds[1].revents & (POLLERR | POLLRDHUP | POLLHUP)) {
      LOG(ERROR, "Broken pipe: %d", read_fd_);
      ret_code = -7;
      break;
    }
  }

  close(timer_fd);
  cleanup();
  return ret_code;
}

void audio_event_handler::handle_request(base::unpacker &pk, uint16_t uri) {
  switch (uri) {
  case LEAVE_REQUEST_URI: {
    p_leave_request req;
    req.unmarshall(pk);
    on_leave();
    break;
  }
  case PHONE_EVENT_CHANGE_URI: {
    p_phone_event event;
    event.unmarshall(pk);
    on_phone_event(event.status);
    break;
  }
  default: break;
  }
}

// ready to leave
void audio_event_handler::on_leave() {
  LOG(INFO, "Ready to leave");
  s_broken_pipe_.store(true);
}

void audio_event_handler::broken_pipe_handler(int sig_no) {
  (void)sig_no;
  assert(sig_no == SIGPIPE);
  s_broken_pipe_.store(true);
}

void audio_event_handler::onJoinChannelSuccess(const char *cname, uid_t uid,
    int elapsed) {
  joined_ = true;
  SAFE_LOG(INFO) << "User " << uid << " successfully joined the channel "
      << cname << ", elapsed " << elapsed << " ms";

  if (!send_join_reply(cname, uid)) {
    LOG(INFO, "Failed to send join reply");
    s_broken_pipe_.store(true);
  }
}

void audio_event_handler::onRejoinChannelSuccess(const char *cname, uid_t uid,
    int elapsed) {
  (void)elapsed;
  SAFE_LOG(INFO) << "User " << uid << " rejoined to the channel " << cname;
}

void audio_event_handler::onWarning(int warn, const char *msg) {
  SAFE_LOG(WARN) << msg << ": " << warn;
}

void audio_event_handler::onError(int err, const char *msg) {
  bool success = true;

  SAFE_LOG(ERROR) << "Unrecoverable error: " << msg << ": " << err;

  switch (err) {
  case EVENT_LOAD_AUDIO_ENGINE_ERROR:
  case EVENT_JOIN_CONNECT_MEDIA_FAILED:
  case EVENT_JOIN_LOGIN_MEDIA_TIMEOUT_ALL:
    success = send_error_report(err, msg);
    break;
  default:
    LOG(INFO, "Error in mediasdk: %d, %s", err, msg);
    break;
  }

  if (!success || err == EVENT_LOAD_AUDIO_ENGINE_ERROR) {
    s_broken_pipe_.store(true);
  }
}

void audio_event_handler::onUserJoined(uid_t uid, int elapsed) {
  SAFE_LOG(INFO) << "User " << uid << " joined the channel, offset "
      << elapsed << " ms";
}

void audio_event_handler::onUserOffline(uid_t uid,
    rtc::USER_OFFLINE_REASON_TYPE reason) {
  SAFE_LOG(INFO) << "User " << uid << " offline, reason: " << reason;
}

void audio_event_handler::onRtcStats(const rtc::RtcStats &stats) {
  if (stats.users > 1) {
    last_active_ts_ = now_ts();
  }
}

void audio_event_handler::onLogEvent(int level, const char *msg, int length) {
  (void)level;
  (void)length;

  LOG(INFO, "%s", msg);
}

bool audio_event_handler::send_join_reply(const char *cname, uint32_t uid) {
  p_join_reply3 reply;
  reply.channel = cname;
  reply.uid = uid;

  return !writer_ || writer_->write_packet(reply) == base::kWriteOk;
}

bool audio_event_handler::send_error_report(int rescode, const char *msg) {
  p_applite_error error;
  error.code = rescode;
  error.info = string(msg);

  return !writer_ || writer_->write_packet(error) == base::kWriteOk;
}

bool audio_event_handler::send_leave_reply() {
  // Not implemented
  return false;
}

void audio_event_handler::onSpeakerJoined(uint32_t uid, const string &path) {
  SAFE_LOG(INFO) << "Speaker " << uid << " joined, write to file: " << path;

  p_speaker_join speaker;
  speaker.uid = uid;
  speaker.path = path;
  if (writer_ && writer_->write_packet(speaker, 4) != base::kWriteOk) {
    SAFE_LOG(ERROR) << "Failed to write speaker info to the peer";
  }
}

void audio_event_handler::on_phone_event(int status) {
  switch (static_cast<PhoneEvent>(status)) {
  case RING:
    LOG(INFO, "Phone is ringing");
    break;
  case PICK_UP:
    LOG(INFO, "Phone is picked up");
    on_pickup_phone();
    break;
  case HANG_UP:
    LOG(INFO, "Phone has been hanged up");
    break;
  default:
    LOG(WARN, "Unknown phone event: %d", status);
    break;
  }
}

void audio_event_handler::on_pickup_phone() {
  applite_->setProfile("{\"mediaSdk\":{\"logCallback\":true, \"dtxMode\":2}}", true);
}

bool audio_event_handler::send_session_creation_event(const char *sessionId) {
  p_session_event session;
  session.session_id = string(sessionId);
  if (!writer_ || writer_->write_packet(session, 4) == base::kWriteOk)
    return true;
  return false;
}

bool audio_event_handler::send_idle_info() {
  p_channel_idle idle;
  idle.idle = channel_idle_;
  if (!writer_ || writer_->write_packet(idle, 10) == base::kWriteOk)
    return true;
  return false;
}

bool audio_event_handler::is_solo_channel() const {
  return last_active_ts_ + channel_idle_ < now_ts();
}

}
}

