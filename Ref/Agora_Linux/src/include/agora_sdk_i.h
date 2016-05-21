#pragma once

#include <cstdint>

namespace agora {
namespace pstn {
class IAgoraSdkEventHandler {
 public:
  enum OfflineReason {kQuit = 0, kDropped = 1};

  // Succeeded in joining the channel |cid|, as the user |uid|.
  virtual void onJoinSuccess(const char *cname, uint32_t uid,
      const char *msg) = 0;

  virtual void onError(int rescode, const char *msg) = 0;
	virtual void onVoiceData(uint32_t uid, const char *pbuffer,
      uint32_t length) = 0;

  // User |uid| has joined the channel.
  virtual void onUserJoined(uint32_t uid) {
    (void)uid;
  }

  // User |uid| has dropped from the channel.
  virtual void onUserOffline(uint32_t uid, OfflineReason reason) {
    (void)uid;
    (void)reason;
  }

  // This function will be called after |joinChannel| is called, but just
  // before any other callbacks. That is, it is the first callback
  // that will be triggered.
  virtual void onSessionCreate(const char *sessionId) {
    (void)sessionId;
  }
};

enum PhoneEvent {UNKNOWN = 0, RING = 1, PICK_UP, HANG_UP};

class IAgoraSdk {
 public:
  static IAgoraSdk* createInstance(IAgoraSdkEventHandler *callback);
  static void destroyInstance(IAgoraSdk *instance);

  virtual ~IAgoraSdk(){}

  // NOTE:
  // Agora transporter would use the ports from [minAllowedUdpPort,
  // maxAllowedUdpPort] for communicating with our signal servers,
  // voice servers, etc. This range should be equal to or larger than 5.
  //
  // If those parameters are given zero, Agora transporter will bind the
  // udp socket to the port allocated by the operating system.
  //
  // The parameter |audio_mixed|:
  //   If true, it means that the caller would like to receive raw, unmixed
  //   voice PCM data from each audio stream, if there exist multiple
  //   persons in a room;
  //   Otherwise, mixed audio PCM data is prefered, in some circumstances,
  //   for example, while sending to a downside telephone peer.
  //
  // The parameter |sampleRates| is the voice sample rates by which the caller,
  // or, voice recorder would expect to record voice.
  // The parameter |sampleRates| is allowed to be one of the following values:
  //  8000
  //  16000
  // , other values will render this call fail.
  //
  // The parameter |reserved| is reserved for future use, DO NOT pass any
  // value except 0.
  //
  // If the parameter |linger_time| is given a value larger than 10,
  // this recording agent will quit while only the agent stays in |channelName|
  // over |linger_time| seconds.
  virtual void joinChannel(const char *vendorID, const char *vendorKey,
      const char *channelName, uint32_t uid, uint16_t minAllowedUdpPort=0,
      uint16_t maxAllowedUdpPort=0, bool audio_mixed=true,
      int32_t sampleRates=8000, uint32_t reserved=0, int linger_time=-1) = 0;

  // Preconditions:
  //  |joinChannel| is called and returned
  //  will block until the recording process quits
  virtual void leave() = 0;

  // Preconditions:
  //  |joinChannel| is called and returned
  virtual void sendVoiceData(const char *pbuffer, uint32_t length) = 0;

  // onTimer should be called every 10 milliseconds
  // In this function, voice data from the other side will be pulled from
  // voice Media SDK
  //
  // This function will
  //   1) If a join request is acknowledged, |onJoinSuccess| will be called
  //   2) If some errors occurred, |onError| will be called; otherwise
  //   3) PCM voice received from the network, |onVoiceData| will be called.
  // Preconditions:
  //  |joinChannel| is called and returned
  virtual void onTimer() = 0;

  // Notify the Media SDK of an incoming event of the peer side's phone.
  // Call of this function is optional.
  // Preconditions:
  //  |joinChannel| is called and returned
  virtual void notifyPhoneEvent(PhoneEvent status) = 0;
};

}
}

