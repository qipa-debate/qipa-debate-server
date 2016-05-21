#include "player/engine_driver.h"

#include <poll.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/time.h>

// #include <atomic>
#include <cassert>
#include <csignal>
#include <cstring>
#include <iostream>
#include <thread>

#include "base/atomic.h"
#include "internal/chat_engine_server_i.h"
#include "internal/recap_format.h"
#include "player/record_file_reader.h"

using namespace std;

namespace agora {
using namespace media;
using namespace recorder;

namespace player {
static atomic_bool_t g_quit_flag;
static void interrupt_handler(int sig_no) {
  (void)sig_no;
  g_quit_flag.store(true);
}

static int64_t now_mseconds() {
  timeval t;
  if (0 != gettimeofday(&t, NULL)) {
    return -1;
  }

  return t.tv_sec * 1000 + t.tv_usec / 1000;
}

engine_driver::engine_driver(int32_t samples) :sample_rates_(samples) {
  engine_ = NULL;
  trace_ = NULL;
}

engine_driver::~engine_driver() {
  if (engine_) {
    destroyChatEngine(engine_);
  }
}

bool engine_driver::play_file(const char *filename, const char *output,
    int speed) {
  if (!init_engine())
    return false;

  trace_ = createTraceService();
  trace_->startTrace(this, 0);

  if (0 != engine_->startCall(0)) {
    cerr << "Failed to start call!" << endl;
    return false;
  }

  IAudioEngine *audio = engine_->getAudioEngine();
  audio->setNetEQMinimumPlayoutDelay(100);

  record_file_reader reader;
  if (!reader.open(filename)) {
    return false;
  }

  int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timer_fd < 0) {
    cerr << "Failed to create a timer fd!" << strerror(errno) << endl;
    return false;
  }

  g_quit_flag.store(false);
  signal(SIGINT, interrupt_handler);

  std::thread recording_thread(&engine_driver::record_pcm, this, output, speed);

  pollfd fds[] = {{timer_fd, POLLIN, 0}};
  int nr_events = 0;
  int sleep_ms = 0;

  int64_t base_ts = now_mseconds();

  audio_packet pkt;
  unsigned char *buf = NULL;

  // static const int kGap = 5;
  bool has_packet = reader.get_next_packet(&pkt, reinterpret_cast<void**>(&buf));

  while ((nr_events = poll(fds, 1, sleep_ms)) >= 0) {
    if (g_quit_flag)
      break;

    while (has_packet && pkt.ts_offset / speed < now_mseconds() - base_ts) {
      int ret = audio->receiveNetPacket(0, buf, pkt.payload_size,
          pkt.payload_type, pkt.timestamp, pkt.seq_number);
      if (ret < 0) {
        cerr << "audio engine error: " << ret << endl;
        break;
      }

      delete []buf;
      buf = NULL;

      void **ppbuf = reinterpret_cast<void**>(&buf);
      has_packet = reader.get_next_packet(&pkt, ppbuf);
    }

    if (!has_packet) {
      buf = NULL;
      break;
    }

    sleep_ms = pkt.ts_offset / speed - (now_mseconds() - base_ts) + 1;
    // cerr << "sleep: " << sleep_ms << endl;
    if (sleep_ms < 0)
      sleep_ms = 0;
  }

  g_quit_flag.store(true);

  recording_thread.join();

  delete []buf;
  close(timer_fd);

  return terminate_engine();
}

bool engine_driver::init_engine() {
  engine_ = createChatEngine(NULL, NULL);
  if (!engine_) {
    cerr << "failed to load the chat engine" << endl;
    return false;
  }

  if (engine_->init() != 0) {
    cerr << "Failed to initialize the chat engine" << endl;
    return false;
  }

  IAudioEngine *audio = engine_->getAudioEngine();
  audio->setMuteStatus(true);
  audio->registerTransport(this);
  return true;
}

bool engine_driver::terminate_engine() {
  if (!engine_)
    return true;

  engine_->stopCall();

  IAudioEngine *audio = engine_->getAudioEngine();
  audio->deRegisterTransport();

  engine_->terminate();

  destroyChatEngine(engine_);
  engine_ = NULL;

  return true;
}

bool engine_driver::record_pcm(const char *output, int speed) {
  bool result = true;

  FILE *fp = fopen(output, "wb");
  if (!fp) {
    cerr << "failed to open " << output << " to write pcm: "
        << strerror(errno) << endl;
    return false;
  }

  IAudioEngine *audio = engine_->getAudioEngine();

  int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timer_fd < 0) {
    cerr << "Failed to create a timer fd!" << strerror(errno) << endl;
    fclose(fp);
    return false;
  }

  const unsigned int nr_samples = sample_rates_ / 100;

  char data[512];
  unsigned int len = 0;
  int nr_events = 0;
  pollfd fds[] = {{timer_fd, POLLIN, 0}};

  int count = 0;
  // int left = 0;
  const int interval_ms = 10 / speed;

  itimerspec timer = {{0, interval_ms * 1000 * 1000}, {0, interval_ms * 1000 * 1000}};
  timerfd_settime(timer_fd, 0, &timer, NULL);

  while ((nr_events = poll(fds, 1, -1)) >= 0 ||
      (nr_events == -1 && errno == EINTR)) {
    if (g_quit_flag)
      break;

    uint64_t expirations = 1;
    int val = read(timer_fd, &expirations, sizeof(uint64_t));
    (void)val;
    // left += expirations - 1;
    // cout << "ret: " << val << ", expire: " << expirations << endl;

    bool failed = false;
    for (int i = 0; i < int(expirations); ++i) {
      int ret = audio->NeedMorePlayData(nr_samples, kBytesPerSample, kChannels,
          sample_rates_, data, len);
      if (ret) {
        cerr << "No Data Available!: " << ret << endl;
        continue;
      }

      assert(len > 0);
      assert(len == nr_samples);
      ++count;
      if (fwrite(data, kBytesPerSample * len, 1, fp) != 1) {
        g_quit_flag = true;
        result = false;
        cerr << "Failed to write to file: " << strerror(errno) << endl;
        failed = true;
        break;
      }
    }

    if (failed) break;
  }

  // cout << "Total packets " << count  << ", left: " << left << endl;
  close(timer_fd);
  fclose(fp);
  return result;
}

int engine_driver::sendAudioPacket(const unsigned char *payloadData,
    unsigned short payloadSize, int payload_type, int vad_type,
    unsigned int timeStamp, unsigned short seqNumber) {
  (void)payloadData;
  (void)payloadSize;
  (void)payload_type;
  (void)vad_type;
  (void)timeStamp;
  (void)seqNumber;

  return 0;
}

void engine_driver::Print(int level, const char *message, int length) {
  (void)length;
  cout << "level: " << level << ": " << message << endl;
}

}
}

