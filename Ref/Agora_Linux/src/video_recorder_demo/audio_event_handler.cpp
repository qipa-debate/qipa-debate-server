#include <sys/time.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <signal.h>
#include <cstring>
#include <cinttypes>
#include <iostream>

#ifdef GOOGLE_PROFILE_FLAG
#include <gperftools/profiler.h>
#endif

#include "base/safe_log.h"
#include "base/packer.h"
#include "base/process.h"
#include "include/error_code.h"
#include "protocol/app_comm.h"
#include "video_recorder_demo/audio_event_handler.h"

using namespace std;

extern bool g_record_pcm;
extern std::string g_record_directory;

extern "C" int registerAudioFrameObserver(agora::media::IAudioFrameObserver *);
extern "C" const char* getChatEngineSourceVersion();

namespace agora {
using namespace rtc;
using namespace base;

// A frame of 10 ms
static const int kFrameInterval = 10;

namespace pstn {

// NOTE(liuyong): Improves me
enum PhoneEvent {UNKNOWN = 0, RING = 1, PICK_UP, HANG_UP};

class pcm_recorder {
 public:
  pcm_recorder();
  ~pcm_recorder();

  bool open(const char *file);
  bool write(const void *buf, int len);
 private:
  FILE *file_;
  void *buf_;
};

inline pcm_recorder::pcm_recorder() {
  file_ = NULL;
  buf_ = NULL;
}

inline pcm_recorder::~pcm_recorder() {
  if (file_)
    fclose(file_);

  free(buf_);
}

inline bool pcm_recorder::open(const char *file) {
  if ((file_ = fopen(file, "wb")) == NULL) {
    LOG(ERROR, "Failed to open the file %s to write: %m", file);
    return false;
  }

  int page_size = getpagesize();
  int size = page_size * 128;
  int ret = posix_memalign(&buf_, page_size, size);
  if (ret != 0 || buf_ == NULL) {
    LOG(ERROR, "Failed to allocate paged memory: %d, %p", ret, buf_);
    return false;
  }

  return setvbuf(file_, reinterpret_cast<char *>(buf_), _IOFBF, size) == 0;
}

inline bool pcm_recorder::write(const void *buf, int len) {
  // assert(file_ != NULL);

  if (file_ == NULL)
    return false;

  size_t count = len >> 1;
  return count == fwrite(buf, 2, count, file_);
}

class speaker {
 public:
  explicit speaker(uint32_t uid, const char *store_file=NULL);
  speaker(speaker &&other);
  ~speaker();

  speaker& operator=(speaker &&other);

  bool receive_pcm_data(const void *data, int bytes);
  bool get_pcm_data(void *data, int bytes);

  bool is_expired() const;
 private:
  speaker(const speaker &other) = delete;
  speaker& operator=(const speaker &other) = delete;
 private:
  uint32_t uid_;
  pcm_recorder *writer_;

  std::deque<char> data_;
};

speaker::speaker(uint32_t uid, const char *store_file) :uid_(uid) {
  writer_ = NULL;
  if (store_file != NULL) {
    if ((writer_ = new (std::nothrow)pcm_recorder) == NULL ||
        !writer_->open(store_file)) {
      SAFE_LOG(ERROR) << "Failed to open the recording file: " << store_file;
      return;
    }
  }
}

speaker::speaker(speaker &&other) {
  uid_ = other.uid_;

  writer_ = other.writer_;
  other.writer_ = NULL;

  std::swap(data_, other.data_);
}

speaker::~speaker() {
  delete writer_;
}

speaker& speaker::operator=(speaker &&rhs) {
  if (this == &rhs)
    return *this;

  uid_ = rhs.uid_;
  std::swap(writer_, rhs.writer_);
  std::swap(data_, rhs.data_);

  return *this;
}

bool speaker::receive_pcm_data(const void *data, int bytes) {
  const char *p = reinterpret_cast<const char *>(data);
  data_.insert(data_.end(), p, p + bytes);

  return true;
}

bool speaker::get_pcm_data(void *data, int bytes) {
  if (data_.size() < static_cast<unsigned>(bytes)) {
    return false;
  }

  char *dst = reinterpret_cast<char *>(data);
  std::copy(data_.begin(), data_.begin() + bytes, dst);
  data_.erase(data_.begin(), data_.begin() + bytes);

  if (writer_ != NULL) {
    writer_->write(data, bytes);
  }

  return true;
}

atomic_bool_t audio_event_handler::s_broken_pipe_(false);

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
    int32_t samples, int idle, bool batch, const char *path_prefix, bool pstn)
:read_fd_(read_fd), write_fd_(write_fd), uid_(uid), vendor_key_(vendor_key),
    channel_name_(channel_name), audio_mixed_(mixed), is_batch_mode_(batch),
    sample_rates_(samples), channel_idle_(idle), is_pstn_(pstn) {
  applite_ = NULL;
  joined_ = false;
  up_pcm_recorder_ = NULL;
  reader_ = NULL;
  writer_ = NULL;
  path_prefix_ = path_prefix;

  if (read_fd >= 0) {
    reader_ = new (std::nothrow)base::pipe_reader(read_fd);
  }

  if (write_fd > 0) {
    writer_ = new (std::nothrow)base::pipe_writer(write_fd);
  }
}

audio_event_handler::~audio_event_handler() {
  if (applite_) {
    cleanup();
  }

  delete reader_;
  delete writer_;
  delete up_pcm_recorder_;
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
  LOG(INFO, "Leaving channel and ready to cleanup");

  applite_->leaveChannel();
  applite_->release();
  applite_ = NULL;

  sleep(1);

  typedef unordered_map<unsigned, speaker *>::iterator iter_t;
  for (iter_t it = speakers_.begin(); it != speakers_.end(); ++it) {
    delete it->second;
  }
  speakers_.clear();
}

bool audio_event_handler::fetch_voice_data() {
  const unsigned int nr_samples = sample_rates_ * kFrameInterval / 1000;
  const unsigned int len = kBytesPerSample * nr_samples;

  vector<char> buffer(len);
  std::lock_guard<std::mutex> lock(pcm_mutex_);

  typedef unordered_map<unsigned, speaker *>::const_iterator iter_t;
  for (iter_t it = speakers_.begin(); it != speakers_.end(); ++it) {
    if (it->second->get_pcm_data(&buffer[0], len) && !send_voice_data(
        it->first, &buffer[0], len)) {
      LOG(ERROR, "Failed to send voice data to PSTN: %m");
      return false;
    }
  }

  return true;
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

  const char *rtcVersion = applite_->getSourceVersion();
  LOG(INFO, "Rtc engine version: %s; chat engine: %s", rtcVersion, getChatEngineSourceVersion());

  RtcEngineContextEx context;
  context.eventHandler = this;
  context.isExHandler = true;
  context.vendorKey = NULL;
  context.context = NULL;

  applite_->initializeEx(context);
  applite_->setLogCallback(true);

  applite_->setProfile("{\"audioEngine\":{\"useAudioExternalDevice\":true}}", true);
  last_active_ts_ = now_ts();

  // setup signal handler
  signal(SIGPIPE, term_handler);
  signal(SIGINT, term_handler);
  signal(SIGTERM, term_handler);

  int64_t join_start_ts = now_us();

  enum {CLIENT_PSTN = 2, CLIENT_RECORDING = 3};
  AParameter param(*applite_);
  param->setInt("rtc.client_type", is_pstn_ ? CLIENT_PSTN : CLIENT_RECORDING);

  const char *codec = NULL;
  switch (sample_rates_) {
    case 16000:
      codec = "NOVA";
      break;
    case 32000:
      codec = "NVWA";
      break;
    default:
      codec = "SILK";
  }

  SAFE_LOG(INFO) << "Set codec to " << codec;
  param->setString("rtc.audio.codec", codec);

  if (applite_->joinChannel(vendor_key_.c_str(), channel_name_.c_str(),
      NULL, uid_) < 0) {
    SAFE_LOG(ERROR) << "Failed to create the channel " << channel_name_;
    return -2;
  }

  if (is_batch_mode_)
    return run_in_batch(join_start_ts);
  return run_interactive(join_start_ts);
}

int audio_event_handler::run_interactive(int64_t join_start_ts) {
  static const int64_t MAX_TIMEOUT = 10 * 1000 * 1000ll;
  static const int32_t ACTIVE_CHECK_TS = 10;

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

  pollfd fds[] = {{timer_fd, POLLIN, 0}, {read_fd_, POLLIN, 0},
    {write_fd_, POLLOUT, 0}};
  int events = 2;

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

        // Fetch voice data and send them to pstn side
        for (unsigned i = 0; i < expirations; ++i) {
          if (!fetch_voice_data()) {
            ret_code = -6;
            goto cleanup;
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

    // if (fds[2].revents & POLLOUT) {
    //   fds[2].revents = 0;
    //   int ret = send_pending_packets();
    //   if (ret == 0) {
    //     // OK, all data has been sent to remote side.
    //     // Disable watching writable events
    //     events = 2;
    //   } else if (ret < 0) {
    //     LOG(ERROR, "Broken pipe: %d", write_fd_);
    //     ret_code = -8;
    //     break;
    //   }
    // } else if (fds[2].revents & (POLLERR | POLLRDHUP | POLLHUP)) {
    //   LOG(ERROR, "Broken pipe: %d", write_fd_);
    //   ret_code = -8;
    //   break;
    // }
  }

cleanup:
  close(timer_fd);
  cleanup();
  return ret_code;
}

int audio_event_handler::run_in_batch(int64_t join_start_ts) {
  static const int64_t MAX_TIMEOUT = 10 * 1000 * 1000ll;
  static const int32_t ACTIVE_CHECK_TS = 10;

  int32_t last_check_ts = now_ts();
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

  pollfd fds[] = {{timer_fd, POLLIN, 0}};

  int ret_code = 0;
  int cnt = -1;

  errno = 0;
  while ((cnt = poll(fds, 1, -1)) >= 0 || (cnt == -1 && errno == EINTR)) {
    if (s_broken_pipe_.load())
      break;

    if (cnt == -1) {
      continue;
    }

    uint64_t expirations = 0;
    ssize_t readed = read(timer_fd, &expirations, sizeof(expirations));
    LOG_IF(ERROR, readed != sizeof(expirations), "Partial read!");
    assert(readed == sizeof(expirations));

    // Check if it is timeout to join the voice server.
    if (!joined_) {
      if (now_us() - join_start_ts < MAX_TIMEOUT)
        continue;
      LOG(WARN, "Max_Timeout elapsed before joinChannel returns");
      ret_code = -4;
      break;
    }

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

    for (unsigned i = 0; i < expirations; ++i) {
      if (!fetch_voice_data()) {
        ret_code = -6;
        goto cleanup;
      }
    }
  }

cleanup:
  close(timer_fd);
  return ret_code;
}

void audio_event_handler::handle_request(base::unpacker &pk, uint16_t uri) {
  switch (uri) {
  case VOICE_DATA_URI: {
    p_voice_data data;
    data.unmarshall(pk);
    LOG_EVERY_N(INFO, 200, "PSTN sends voice data: %" PRIu32,
        uint32_t(data.data.size()));
    on_voice_data(data.data);
    break;
  }
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

// received from PSTN
void audio_event_handler::on_voice_data(const string &data) {
  if (joined_ && data.size() > 0) {
    {
      std::lock_guard<std::mutex> lock(in_mutex_);
      in_data_.insert(in_data_.end(), data.begin(), data.end());
    }

    if (up_pcm_recorder_)
      up_pcm_recorder_->write(&data[0], data.size());
  }
}

// ready to leave
void audio_event_handler::on_leave() {
  LOG(INFO, "Ready to leave");
  s_broken_pipe_.store(true);
}

void audio_event_handler::term_handler(int sig_no) {
  (void)sig_no;
  s_broken_pipe_.store(true);
}

void audio_event_handler::onError(int rescode, const char *msg) {
  bool success = true;
  switch (rescode) {
  case EVENT_LOAD_AUDIO_ENGINE_ERROR:
  case EVENT_JOIN_CONNECT_MEDIA_FAILED:
  case EVENT_JOIN_LOGIN_MEDIA_TIMEOUT_ALL:
    success = send_error_report(rescode, msg);
    break;
  default:
    LOG(INFO, "Error in mediasdk: %d, %s", rescode, msg);
    break;
  }

  if (!success || rescode == EVENT_LOAD_AUDIO_ENGINE_ERROR) {
    s_broken_pipe_.store(true);
  }
}

void audio_event_handler::onJoinChannelSuccess(const char *channel, uid_t uid,
    int ts) {
  SAFE_LOG(INFO) << uid << " logined successfully in " << channel
      << ", elapsed: " << ts << " ms";

  // NOTE(liuyong): Once joined_ set to true, |run| will handle the input voice
  // data, and try to write to recording files, so we must open the recorders
  // before this flag is set.

  // open the upstream pcm recorder to store pcm data.
  string path;
  if (!joined_ && get_recording_path(0, now_us(), false, &path)) {
    delete up_pcm_recorder_;
    if ((up_pcm_recorder_ = new (std::nothrow)pcm_recorder) == NULL ||
        !up_pcm_recorder_->open(path.c_str())) {
      SAFE_LOG(ERROR) << "Failed to open the recording file: " << path;
    }
  }

  joined_ = true;
  registerAudioFrameObserver(this);

  LOG(INFO, "User %u join channel %s success", uid, channel);

  if (!send_join_reply(channel, uid)) {
    LOG(INFO, "Failed to send join reply");
    s_broken_pipe_.store(true);
  }
}

void audio_event_handler::onRejoinChannelSuccess(const char *channel, uid_t uid,
    int elapsed) {
  SAFE_LOG(INFO) << uid << " rejoin to channel: " << channel << ", time offset "
      << elapsed << " ms";
}

void audio_event_handler::onWarning(int warn, const char *msg) {
  SAFE_LOG(WARN) << "code: " << warn << ", " << msg;
}

void audio_event_handler::onUserJoined(uid_t uid, int elapsed) {
  SAFE_LOG(INFO) << "offset " << elapsed << " ms: " << uid
      << " joined the channel";
}

void audio_event_handler::onUserOffline(uid_t uid,
    USER_OFFLINE_REASON_TYPE reason) {
  const char *detail = reason == USER_OFFLINE_QUIT ? "quit" : "dropped";
  SAFE_LOG(INFO) << "User " << uid << " " << detail;
}

void audio_event_handler::onRtcStats(const rtc::RtcStats &stats) {
  if (stats.users > 1) {
    last_active_ts_ = now_ts();
  }
}

void audio_event_handler::onLogEvent(int level, const char *msg, int length) {
  (void)length;
  LOG(INFO, "level %d: %s", level, msg);
}

bool audio_event_handler::onRecordFrame(void *audioSample, int nSamples,
    int nBytesPerSample, int nChannels, int sampleRates) {
  if (sampleRates != sample_rates_) {
    SAFE_LOG(WARN) << "Recording sample rates conflict: expected "
        << sample_rates_ << ", while " << sampleRates;
    return false;
  }

  std::lock_guard<std::mutex> lock(in_mutex_);
  size_t bytes = nSamples * nBytesPerSample * nChannels;
  if (in_data_.size() < bytes) {
    return false;
  }

  char *dst = reinterpret_cast<char *>(audioSample);
  std::copy(in_data_.begin(), in_data_.begin() + bytes, dst);
  in_data_.erase(in_data_.begin(), in_data_.begin() + bytes);
  return true;
}

bool audio_event_handler::onPlaybackFrame(void *audioSample, int nSamples,
    int nBytesPerSample, int nChannels, int sampleRates) {
  // Not in mixed mode, just returns.
  if (!audio_mixed_)
    return true;

  if (sampleRates != sample_rates_) {
    SAFE_LOG(WARN) << "Playback sample rates conflict: expected "
        << sample_rates_ << ", while " << sampleRates;
    return false;
  }

  LOG_EVERY_N(INFO, 200, "Audio Engine sends mixed voice data: %" PRIu32,
      uint32_t(nSamples * nBytesPerSample * nChannels));

  std::lock_guard<std::mutex> lock(pcm_mutex_);
  int bytes = nSamples * nBytesPerSample * nChannels;
  const char *voice = reinterpret_cast<const char *>(audioSample);
  static const unsigned kMixedUid = 0;
  auto it = speakers_.find(kMixedUid);

  if (it == speakers_.end()) {
    const char *stored = NULL;
    string path;
    if (get_recording_path(kMixedUid, now_us(), true, &path)) {
      stored = path.c_str();
    }

    speaker *spk = new (std::nothrow)speaker(kMixedUid, stored);
    it = speakers_.insert(make_pair(kMixedUid, spk)).first;
  }

  return it->second->receive_pcm_data(voice, bytes);
}

bool audio_event_handler::onPlaybackFrameUid(unsigned uid, void *audioSample,
    int nSamples, int nBytesPerSample, int nChannels, int sampleRates) {
  // If it is in mixed mode, ignores this invocation.
  if (audio_mixed_)
    return true;

  if (sampleRates != sample_rates_) {
    SAFE_LOG(WARN) << "User " << uid << " playback sample rates conflict: "
      "expected " << sample_rates_ << ", while " << sampleRates;
    return false;
  }

  LOG_EVERY_N(INFO, 200, "User %u sends unmixed voice data: %" PRIu32,
      uid, uint32_t(nSamples * nBytesPerSample * nChannels));

  std::lock_guard<std::mutex> lock(pcm_mutex_);
  auto it = speakers_.find(uid);
  if (it == speakers_.end()) {
    const char *stored = NULL;
    string path;
    if (get_recording_path(uid, now_us(), true, &path)) {
      stored = path.c_str();
    }

    speaker *spk = new (std::nothrow)speaker(uid, stored);
    it = speakers_.insert(make_pair(uid, spk)).first;
  }

  int bytes = nSamples * nBytesPerSample * nChannels;
  return it->second->receive_pcm_data(audioSample, bytes);
}

bool audio_event_handler::send_voice_data(unsigned int uid, const char *buf,
    int len) {
  if (!writer_)
    return true;

  p_voice_data2 data;
  data.uid = uid;
  data.data = string(buf, len);

  static const int kMaxRetry = 4;
  base::WriteState state = writer_->write_packet(data, kMaxRetry);
  LOG_EVERY_N(INFO, 200, "send voice data to PSTN. state: %d", state);
  switch (state) {
  case base::kRetryExceeded:
    LOG(INFO, "Buffer to PSTN overrun");
    break;
  case base::kWriteError: return false;
  default: break;
  }

  return true;
}

bool audio_event_handler::send_join_reply(const char *channel, uint32_t uid) {
  if (!writer_)
    return true;

  p_join_reply3 reply;
  reply.channel = channel;
  reply.uid = uid;

  return writer_->write_packet(reply) == base::kWriteOk;
}

bool audio_event_handler::send_error_report(int rescode, const char *msg) {
  if (!writer_)
    return true;

  p_applite_error error;
  error.code = rescode;
  error.info = string(msg);

  return writer_->write_packet(error) == base::kWriteOk;
}

bool audio_event_handler::send_leave_reply() {
  // Not implemented
  return false;
}

bool audio_event_handler::get_recording_path(unsigned uid, int64_t ts,
    bool down, string *path) const {
  char location[512];

  if (is_batch_mode_) {
    // No upstream in such a mode
    // assert(down);
    if (!down) return false;

    snprintf(location, 511, "%s%s-%u.pcm", path_prefix_.c_str(),
        channel_name_.c_str(), uid);
    *path = location;
    return true;
  }

  // only records mixed voice data.
  if (!g_record_pcm || uid != 0) {
    return false;
  }

  snprintf(location, 511, "%s/%" PRId64 "_%s_%s_%s.pcm",
      g_record_directory.c_str(),
      ts, channel_name_.c_str(), vendor_key_.c_str(),
      down ? "down" : "up");
  *path = location;
  return true;
}

// void audio_event_handler::onPeerConnected(int callSetupTime) {
//   LOG(INFO, "Elapsed time for call setup: %d ms", callSetupTime);
// }

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
  if (!writer_)
    return true;

  p_session_event session;
  session.session_id = string(sessionId);
  if (writer_->write_packet(session, 4) == base::kWriteOk)
    return true;
  return false;
}

bool audio_event_handler::send_idle_info() {
  if (!writer_)
    return true;

  p_channel_idle idle;
  idle.idle = channel_idle_;
  if (writer_->write_packet(idle, 10) == base::kWriteOk)
    return true;
  return false;
}

bool audio_event_handler::is_solo_channel() const {
  return last_active_ts_ + channel_idle_ < now_ts();
}

int audio_event_handler::send_pending_packets() {
  base::WriteState state;
  while (!pending_packets_.empty()) {
    const packet_buffer_t &pkt = pending_packets_.front();
    if ((state = writer_->write_buffer(&pkt[0], pkt.size())) != base::kWriteOk)
      break;
    pending_packets_.pop();
  }

  if (state == base::kWriteError)
    return -1;
  return static_cast<int>(pending_packets_.size());
}

}
}

