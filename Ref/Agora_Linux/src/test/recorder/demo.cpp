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
#include <vector>

#include "recording_sdk.h"

using namespace std;
using namespace agora;
using namespace agora::recorder;

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
#endif

atomic_bool_t g_quit_flag;

struct EventHandler : public IRecordEventHandler {
 public:
  explicit EventHandler(atomic_bool_t &joined_flag);
  ~EventHandler();

  virtual void onJoinSuccess(const char *name, unsigned uid, const char *msg);
  virtual void onError(int error_code, const char *msg);
  virtual void onSessionCreate(const char *sessionId);
  virtual void onSpeakerJoined(unsigned uid, const char *path);
 private:
  atomic_bool_t &joined_flag_;
};

EventHandler::EventHandler(atomic_bool_t &joined_flag)
    :joined_flag_(joined_flag) {
}

EventHandler::~EventHandler() {
}

void EventHandler::onJoinSuccess(const char *name, unsigned uid, const char *msg) {
  joined_flag_ = true;
  LOG(INFO, "User %u joined the channel %s: %s", uid, name, msg);
}

void EventHandler::onError(int rescode, const char *msg) {
  LOG(ERROR, "Error: %d, %s", rescode, msg);
  g_quit_flag = true;
}

void EventHandler::onSessionCreate(const char *sessionId) {
  LOG(INFO, "Session created: %s", sessionId);
}

void EventHandler::onSpeakerJoined(unsigned uid, const char *path) {
  LOG(INFO, "Speaker %u joined. Voice will be saved on %s", uid, path);
}

void interrupt_handler(int sig_no) {
  (void)sig_no;
  g_quit_flag = true;
}

int main(int argc, char *argv[]) {
  g_quit_flag = false;
  signal(SIGINT, interrupt_handler);

  // SIGPIPE Must be captured.
  signal(SIGPIPE, SIG_IGN);

  atomic_bool_t joined_flag;
  joined_flag = false;

  EventHandler callback(joined_flag);
  IRecordSdk *sdk = IRecordSdk::createInstance(&callback);
  if (!sdk) {
    LOG(ERROR, "Failed to create the instance of AgoraSdk!");
    return -1;
  }

  int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timer_fd < 0) {
    LOG(FATAL, "Failed to create a timer fd!");
    IRecordSdk::destroyInstance(sdk);
    return -1;
  }

  uint32_t uid = 0; // Got from signal services.
  const char vendor_key[] = "6D7A26A1D3554A54A9F43BE6797FE3E2";
  const char channel_name[] = "9999";
  LOG(INFO, "joining channel: %s, key: %s\n", channel_name, vendor_key);

  sdk->joinChannel(NULL, vendor_key, channel_name, uid, 0, 0, "./");

  // Invoke |onTimer| every 10 ms
  itimerspec spec = {{0, 10 * 1000 * 1000}, {0, 10 * 1000 * 1000}};
  timerfd_settime(timer_fd, 0, &spec, NULL);

  // wait for reply from the voice server
  pollfd fds[] = {{timer_fd, POLLIN, 0}};
  int nr_events = 0;

  while ((nr_events = poll(fds, 1, -1)) >= 0 ||
      (nr_events == -1 && errno == EINTR)) {
    // Fetch events from the SDK
    sdk->onTimer();

    if (g_quit_flag)
      break;

    if (!joined_flag)
      continue;
  }

  close(timer_fd);

  LOG(INFO, "leaving......\n");
  sdk->leave();
  IRecordSdk::destroyInstance(sdk);

  return 0;
}

