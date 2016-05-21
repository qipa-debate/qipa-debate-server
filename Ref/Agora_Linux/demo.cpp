#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <signal.h>

#define __STDC_FORMAT_MACROS 1

#include <inttypes.h>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include "agora_sdk_i.h"
// #include "base/log.h"

using namespace std;
using namespace agora;
using namespace agora::pstn;

inline int64_t now_ms() {
  timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000ll + now.tv_usec / 1000;
}

#if !defined(LOG)
#include <cstdio>
#define LOG(level, fmt, ...) printf("%" PRId64 ": " fmt "\n", now_ms(), ## __VA_ARGS__)
#endif

#ifdef __GNUC__

#if __GNUC_PREREQ(4, 6)
#include <atomic>
typedef std::atomic<bool> atomic_bool_t;
#else
typedef volatile bool atomic_bool_t;
#endif

#if __GNUC_PREREQ(4, 7)
#define OVERRIDE override
#else
#define OVERRIDE
#endif

#endif

atomic_bool_t g_quit_flag;

class AgoraChannel : private IAgoraSdkEventHandler {
 public:
  AgoraChannel(const string &vendorKey, const string &channelName, uint32_t uid=0,
      bool audioMixed=true, int32_t sampleRates=8000);

  virtual ~AgoraChannel();

  // Spawn a thread to run the loop of fetching data.
  // return immediately
  void Start(const char *pcm_file=NULL);

  // Leave the channel and stop the worker thread.
  // not return until the worker thread quits the channel.
  void Stop();

  // Query the channel state.
  // bool GetChannelState() const;
  bool isClosed() const;
 private:
  virtual void onJoinSuccess(const char *cname, unsigned uid, const char *msg) OVERRIDE;
  virtual void onError(int rescode, const char *msg) OVERRIDE;
  virtual void onVoiceData(uint32_t uid, const char *pbuffer, uint32_t length) OVERRIDE;
  virtual void onSessionCreate(const char *sessionId) OVERRIDE;
 private:
  int Run();
 private:
  string vendor_key_;
  string channel_name_;
  uint32_t uid_;
  bool audio_mixed_;
  int32_t sample_rates_;
  uint32_t last_active_ts_;

  const char *input_file_;
  FILE *pcm_file_;
  atomic_bool_t joined_flag_;
  atomic_bool_t quit_flag_;

  std::thread worker_;
};

AgoraChannel::AgoraChannel(const string &vendorKey, const string &channelName,
    uint32_t uid, bool audioMixed, int32_t sampleRates)
    :vendor_key_(vendorKey), channel_name_(channelName), uid_(uid),
    audio_mixed_(audioMixed), sample_rates_(sampleRates) {
  string file_name = "./" + channelName + ".pcm";
  pcm_file_ = fopen(file_name.c_str(), "wb");
  if (!pcm_file_) {
    LOG(FATAL, "Failed to open pcm file to write: %s",
        strerror(errno));
    abort();
  }

  input_file_ = NULL;

  quit_flag_ = false;
  joined_flag_ = false;
  last_active_ts_ = static_cast<uint32_t>(now_ms() / 1000);
}

AgoraChannel::~AgoraChannel() {
  fclose(pcm_file_);
}

void AgoraChannel::onJoinSuccess(const char *cname, unsigned uid, const char *msg) {
  joined_flag_ = true;
  LOG(INFO, "User %u joined the channel %s: %s", uid, cname, msg);
}

void AgoraChannel::onError(int rescode, const char *msg) {
  LOG(ERROR, "Error: %d, %s", rescode, msg);
  quit_flag_ = true;
}

void AgoraChannel::onVoiceData(unsigned int uid, const char *buf,
    uint32_t length) {
  (void)uid;
  (void)buf;
  (void)length;
  // LOG(INFO, "voice data received: %d, from %d", int(length), int(uid));
  fwrite(buf, 1, length, pcm_file_);
}

void AgoraChannel::onSessionCreate(const char *sessionId) {
  LOG(INFO, "Session created: %s", sessionId);
}

void AgoraChannel::Start(const char *input_file) {
  input_file_ = input_file;
  worker_ = std::thread(&AgoraChannel::Run, this);
}

int AgoraChannel::Run() {
  // is_running_ = true;
  joined_flag_ = false;

  size_t samples = 2 * 1024 * 1024;
  vector<char> pcms;
  pcms.resize(samples);

  if (input_file_) {
    FILE *input = NULL;
    if ((input = fopen(input_file_, "rb")) == NULL) {
      LOG(ERROR, "Opening the input PCM file %s failed: %s", input_file_, strerror(errno));
    } else {
      size_t readed = fread(&pcms[0], 1, samples, input);
      pcms.resize(readed);
      fclose(input);
    }
  }

  IAgoraSdk *sdk = IAgoraSdk::createInstance(this);
  if (!sdk) {
    LOG(ERROR, "Failed to create the instance of AgoraSdk while joining the "
        "channel %s", channel_name_.c_str());
    return -1;
  }

  // create a timer for polling voice data.
  int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timer_fd < 0) {
    LOG(FATAL, "Failed to create a timer fd!");
    IAgoraSdk::destroyInstance(sdk);
    return -1;
  }

  LOG(INFO, "joining channel: %s, key: %s\n", channel_name_.c_str(),
      vendor_key_.c_str());

  sdk->joinChannel(NULL, vendor_key_.c_str(), channel_name_.c_str(), uid_,
      0, 0, audio_mixed_, sample_rates_, 0, 20);

  // Invoke |onTimer| every 10 ms
  itimerspec spec = {{0, 10 * 1000 * 1000}, {0, 10 * 1000 * 1000}};
  timerfd_settime(timer_fd, 0, &spec, NULL);

  // wait for reply from the voice server
  pollfd fds[] = {{timer_fd, POLLIN, 0}};
  int nr_events = 0;
  int frame_no = 0;

  while ((nr_events = poll(fds, 1, -1)) >= 0 ||
      (nr_events == -1 && errno == EINTR)) {
    // Fetch voice data from the SDK
    sdk->onTimer();

    if (quit_flag_)
      break;

    uint64_t expirations = 1;
    int val = read(timer_fd, &expirations, sizeof(expirations));
    (void)val;
    assert(val == sizeof(expirations));

    if (!joined_flag_)
      continue;

    // 80 samples per 10 miliseconds for 8K rate.
    static const size_t FRAME_SIZE = 80 * 2;
    size_t total = pcms.size() / FRAME_SIZE;
    if (total == 0)
      continue;

    for (int i = 0; i < int(expirations); ++i) {
      const char *buf = reinterpret_cast<const char *>(&pcms[frame_no * FRAME_SIZE]);

      // TODO: Fill this buffer with real PSTN voice
      sdk->sendVoiceData(buf, FRAME_SIZE);
      frame_no = (frame_no + 1) % total;
    }
  }

  close(timer_fd);

  LOG(INFO, "leaving......\n");
  sdk->leave();

  IAgoraSdk::destroyInstance(sdk);
  joined_flag_ = false;

  return 0;
}

void AgoraChannel::Stop() {
  quit_flag_ = true;
  worker_.join();
}

bool AgoraChannel::isClosed() const {
  return quit_flag_;
}

void interrupt_handler(int sig_no) {
  (void)sig_no;
  g_quit_flag = true;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  g_quit_flag = false;
  signal(SIGINT, interrupt_handler);

  // SIGPIPE Must be captured.
  signal(SIGPIPE, SIG_IGN);

  // std::vector<AgoraChannel *> channels;
  // const char * const cnames[] = {"Recording_test_1"};
  // const char * const cnames[] = {
  //   "Recording_test_1",
  //   "Recording_test_2",
  //   "Recording_test_3"
  // };

  static const char vendor_key[] = "YOUR_KEY";
  AgoraChannel *p = new (std::nothrow)AgoraChannel(vendor_key,
      "802", 1234567, true, 8000);

  const char *pcm_file = argc > 1 ? argv[1] : NULL;

  p->Start(pcm_file);

  while (!g_quit_flag && !p->isClosed()) {
    sleep(2);
  }

  p->Stop();
  delete p;

  // for (unsigned i = 0; i < sizeof(cnames) / sizeof(cnames[0]); ++i) {
  //   AgoraChannel *p = new (std::nothrow)AgoraChannel(vendor_key,
  //       cnames[i], 0, true, 16000);
  //   p->Start();
  //   channels.push_back(p);
  // }

  // sleep(30);

  // for (unsigned i = 0; i < sizeof(cnames) / sizeof(cnames[0]); ++i) {
  //   AgoraChannel *p = channels[i];
  //   p->Stop();
  //   delete p;
  // }

  return 0;
}

