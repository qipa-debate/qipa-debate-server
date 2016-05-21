#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <signal.h>

#define __STDC_FORMAT_MACROS 1

#include <inttypes.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "agora_sdk_i.h"
#include "base/atomic.h"
#include "base/log.h"
#include "base/opt_parser.h"

using namespace std;
using namespace agora;
using namespace agora::pstn;

inline int64_t now_us() {
  timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000ll * 1000 + now.tv_usec;
}

// #if !defined(LOG)
// #include <cstdio>
// #define LOG(level, fmt, ...) printf("%" PRId64 ": " fmt "\n", now_us(), ## __VA_ARGS__)
// #endif

// #ifdef __GNUC__
//
// #if __GNUC_PREREQ(4, 6)
//
// #include <atomic>
// typedef std::atomic<bool> atomic_bool_t;
// #else
// typedef volatile bool atomic_bool_t;
//
// #endif
// #endif

atomic_bool_t g_quit_flag;

class PstnThread  : private IAgoraSdkEventHandler {
 public:
  PstnThread(const string &channel, uint16_t min_port, uint16_t max_port);
  virtual ~PstnThread();

  int run(const string &input_file, const string &output_file);
 private:
  PstnThread(const PstnThread &) = delete;
  PstnThread(PstnThread &&) = delete;
  PstnThread& operator=(const PstnThread &) = delete;
  PstnThread& operator=(PstnThread &&) = delete;
 private:
  bool load_voice(const string &input);
 private:
  virtual void onJoinSuccess(const char *cname, unsigned uid, const char *msg);
  virtual void onError(int rescode, const char *msg);
  virtual void onVoiceData(uint32_t uid, const char *pbuffer, uint32_t length);
 private:
  uint16_t min_port_;
  uint16_t max_port_;

  string channel_name_;
  vector<uint16_t> pcm_data_;
  FILE *pcm_file_;
  atomic<bool> quit_flag_;
  atomic<bool> joined_flag_;
};

PstnThread::PstnThread(const string &channel, uint16_t min_port, uint16_t max_port) {
  min_port_ = min_port;
  max_port_ = max_port;
  channel_name_ = channel;

  joined_flag_ = false;
  quit_flag_ = false;
  pcm_file_ = NULL;
}

bool PstnThread::load_voice(const string &input) {
  FILE *fp = fopen(input.c_str(), "rb");
  if (!fp) {
    LOG(ERROR, "Failed to open the pcm file: %s, Reason: %s", input.c_str(),
        strerror(errno));
    return false;
  }

  struct stat info;
  if (fstat(fileno(fp), &info) == -1) {
    LOG(ERROR, "Failed to get the pcm file size: %s", strerror(errno));
    fclose(fp);
    return false;
  }

  if (info.st_size > 0) {
    size_t cnt = info.st_size / sizeof(uint16_t);
    pcm_data_.resize(cnt);
    size_t n = fread(&pcm_data_[0], sizeof(uint16_t), cnt, fp);
    pcm_data_.resize(n);
  }

  fclose(fp);
  return true;
}

int PstnThread::run(const string &input, const string &out) {
  pcm_file_ = fopen(out.c_str(), "wb");
  if (!pcm_file_) {
    LOG(FATAL, "Failed to open pcm file to write: %s",
        strerror(errno));
    // abort();
    return -1;
  }

  if (!load_voice(input))
    return -1;

  IAgoraSdk *sdk = IAgoraSdk::createInstance(this);
  if (!sdk) {
    LOG(ERROR, "Failed to create the instance of AgoraSdk!");
    return -1;
  }

  int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timer_fd < 0) {
    LOG(FATAL, "Failed to create a timer fd!");
    IAgoraSdk::destroyInstance(sdk);
    return -1;
  }

  uint32_t uid = 0; // Got from signal services.
  const char vendor_key[] = "6D7A26A1D3554A54A9F43BE6797FE3E2";
  LOG(INFO, "joining channel: %s with key(%s)\n", channel_name_.c_str(), vendor_key);

  sdk->joinChannel(NULL, vendor_key, channel_name_.c_str(), uid,
      min_port_, max_port_);

  // wait for reply from the voice server
  pollfd fds[] = {{timer_fd, POLLIN, 0}};
  int nr_events = 0;
  int frame_no = 0;

  while ((nr_events = poll(fds, 1, 10)) >= 0) {
    // Fetch voice data from the SDK
    sdk->onTimer();

    if (g_quit_flag)
      break;

    if (!joined_flag_)
      continue;

    // 80 samples per 10 miliseconds for 8K rate.
    static const size_t SAMPLES_COUNT = 80;
    size_t total = pcm_data_.size() / SAMPLES_COUNT;
    if (total == 0)
      continue;

    // LOG(INFO, "Sending buffer from %s: %d\n", channel_name_.c_str(), frame_no);

    const char *buf = reinterpret_cast<const char *>(&pcm_data_[frame_no * SAMPLES_COUNT]);
    // Fill this buffer with real PSTN voice
    sdk->sendVoiceData(buf, 160);
    frame_no = (frame_no + 1) % total;
  }

  close(timer_fd);

  LOG(INFO, "leaving channel %s......", channel_name_.c_str());
  sdk->leave();
  IAgoraSdk::destroyInstance(sdk);

  return 0;
}

PstnThread::~PstnThread() {
  if (pcm_file_)
    fclose(pcm_file_);
}

void PstnThread::onJoinSuccess(const char *cname, unsigned uid, const char *msg) {
  (void)cname;
  (void)uid;
  joined_flag_.store(true);

  LOG(INFO, "Joined the channel %s: %s", cname, msg);
}

void PstnThread::onError(int rescode, const char *msg) {
  LOG(ERROR, "Error in channel %s: %d, %s", channel_name_.c_str(), rescode, msg);
  g_quit_flag = true;
}

void PstnThread::onVoiceData(uint32_t uid, const char *buf, uint32_t length) {
  (void)uid;
  (void)buf;
  (void)length;
  // LOG(INFO, "voice data received: %u", length);
  fwrite(buf, 1, length, pcm_file_);
}

void interrupt_handler(int sig_no) {
  (void)sig_no;
  g_quit_flag = true;
}

int main(int argc, char *argv[]) {
  int duplex_count = 100;
  if (argc == 2) {
    duplex_count = atoi(argv[1]);
  }

  g_quit_flag = false;
  signal(SIGINT, interrupt_handler);

  // SIGPIPE Must be captured.
  signal(SIGPIPE, SIG_IGN);

  vector<PstnThread *> lines;
  vector<thread> threads;
  uint16_t min_port = 7000;
  uint16_t max_port = 7005;
  const string input("./voice5.pcm");
  const char channel_base[] = "pstn_test_";

  for (int i = 0; i < duplex_count; ++i) {
    char buf[64];
    snprintf(buf, 64, "%s%d", channel_base, i);
    PstnThread *p = new PstnThread(buf, min_port, max_port);

    snprintf(buf, 64, "output%d.pcm", i);
    lines.push_back(p);
    thread t(&PstnThread::run, p, input, string(buf));
    threads.push_back(move(t));

    min_port += 10;
    max_port += 10;
  }

  for (int i = 0; i < duplex_count; ++i) {
    threads[i].join();
    delete lines[i];
  }

  return 0;
}

