#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

#include "internal/rtc/IAgoraRtcEngine.h"
#include "internal/rtc/rtc_engine_i.h"

using namespace std;
using namespace agora::rtc;
using namespace agora::media;

extern "C" int registerAudioFrameObserver(agora::media::IAudioFrameObserver *observer);

class RtcChannel : private IRtcEngineEventHandler, private IAudioFrameObserver {
 public:
  explicit RtcChannel(const char *vendor_key);
  ~RtcChannel();

  bool join(const char *channel_name);
  bool joined() const;

  bool leave();

  int run();
 private:
  static void signal_handler(int signo);
 private:
  virtual void onJoinChannelSuccess(const char *cid, uid_t uid, int elapsed);
  virtual void onRejoinChannelSuccess(const char *cid, uid_t uid, int elapsed);
  virtual void onWarning(int warn, const char *msg);
  virtual void onError(int err, const char *msg);
  virtual void onUserJoined(uid_t uid, int elapsed);
	virtual void onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason);

  // inherited from IAudioFrameObserver
  virtual bool onRecordFrame(void *audioSample, int nSamples,
      int nBytesPerSample, int nChannels, int samplesPerSec);
  virtual bool onPlaybackFrame(void *audioSample, int nSamples,
      int nBytesPerSample, int nChannels, int samplesPerSec);
 private:
  string vendor_key_;
  IRtcEngineEx *eng_;

  FILE *fp_;
  std::atomic<bool> joining_;
  static std::atomic<bool> quit_flag_;
};

std::atomic<bool> RtcChannel::quit_flag_;

RtcChannel::RtcChannel(const char *vendor_key) : vendor_key_(vendor_key) {
  joining_ = false;
  quit_flag_ = false;

  if ((fp_ = fopen("./out.pcm", "wb")) == NULL) {
    cerr << "Fatal error: " << strerror(errno) << endl;
    return;
  }

  eng_ = static_cast<IRtcEngineEx*>(createAgoraRtcEngine());
  if (!eng_) {
    cerr << "Failed to create the rtc engine!" << endl;
  }

  assert(eng_ != NULL);
  RtcEngineContext context = {this, vendor_key};
  if (eng_->initialize(context)) {
    cerr << "Failed to initialize the chat engine!" << endl;
  }

  eng_->setProfile("{\"audioEngine\":{\"useAudioExternalDevice\":true}}", true);
}

RtcChannel::~RtcChannel() {
  if (eng_) {
    eng_->release();
  }
}

void RtcChannel::onJoinChannelSuccess(const char *channel, uid_t uid, int ts) {
  cout << "logging success: " << channel << ", " << uid << ", ts: "
      << ts << endl;
  RtcEngineParameters mute(*eng_);
  mute.muteLocalAudioStream(true);

  registerAudioFrameObserver(this);
}

void RtcChannel::onRejoinChannelSuccess(const char *channel, uid_t uid,
    int elapsed) {
  cout << "rejoin to channel " << channel << ", uid: " << uid
      << ", established time: " << elapsed << endl;
}

void RtcChannel::onWarning(int warn, const char *msg) {
  cout << "Warning: " << warn << endl; // << ", " << msg << endl;
}

void RtcChannel::onError(int err, const char *msg) {
  // quit_flag_ = true;
  cerr << "Error: " << err << ", " << msg << endl;
}

void RtcChannel::onUserJoined(uid_t uid, int elapsed) {
  (void)elapsed;
  cout << "User " << uid << " joined the channel" << endl;
}

void RtcChannel::onUserOffline(uid_t uid, USER_OFFLINE_REASON_TYPE reason) {
  cout << "User " << uid << " dropped: " << reason << endl;
}

bool RtcChannel::onRecordFrame(void *audioSample, int nSamples,
    int nBytesPerSample, int nChannels, int samplesPerSec) {
  // cout << "require voice data " << nSamples << ", " << samplesPerSec << endl;
  return false;
}

bool RtcChannel::onPlaybackFrame(void *audioSample, int nSamples,
    int nBytesPerSample, int nChannels, int samplesPerSec) {
  // cout << "voice data: " << "bits: " << nBytesPerSample * 8 << ",sample: " << nSamples << ", " << samplesPerSec << ", channel: " << nChannels << endl;
  fwrite(audioSample, 1, nSamples * nBytesPerSample * nChannels, fp_);
  return true;
}

bool RtcChannel::join(const char *channel_name) {
  signal(SIGINT, signal_handler);

  AParameter param(*eng_);
  param->setString("rtc.audio.codec", "SILK");

  joining_ = true;
  return 0 == eng_->joinChannel(vendor_key_.c_str(), channel_name, NULL, 0);
}

// bool RtcChannel::joined() const {
//   return false;
// }

bool RtcChannel::leave() {
  quit_flag_ = true;
  return true;
}

int RtcChannel::run() {
  while (!quit_flag_) {
    sleep(1);
  }

  eng_->leaveChannel();
  return 0;
}

void RtcChannel::signal_handler(int signo) {
  quit_flag_ = true;
}

int main(int argc, char *argv[]) {
  RtcChannel channel("f4637604af81440596a54254d53ade20");
  channel.join("803");
  channel.run();
  return 0;
}

