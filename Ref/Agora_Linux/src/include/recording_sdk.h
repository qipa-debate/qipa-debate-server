#pragma once

#include <cstdint>

namespace agora {
namespace recorder {
class IRecordEventHandler {
 public:
  // Succeeded in joining the channel |cname|, as the user |uid|.
  virtual void onJoinSuccess(const char *cname, unsigned uid, const char *msg)= 0;
  virtual void onError(int error_code, const char *msg) = 0;

  // This function will be called after |joinChannel| is called, but just
  // before any other callbacks. That is, it is the first callback
  // that will be triggered.
  virtual void onSessionCreate(const char *sessionId) {
    (void)sessionId;
  }

  // First voice frame received from the speaker |uid|
  virtual void onSpeakerJoined(unsigned uid, const char *path) {
    (void)uid;
    (void)path;
  }
};

class IRecordSdk {
 public:
  static IRecordSdk* createInstance(IRecordEventHandler *callback);
  static void destroyInstance(IRecordSdk *instance);

  virtual ~IRecordSdk(){}

  // Call this function to join the channel |channelName|, receive and
  // store the encoded voice data into a record file named by
  // |channelName|-|uid|-timestamp.agr in the path |dir_prefix|,
  // if |dir_prefix| is not NULL, where
  //   |uid| is the unique id of the speaker; and
  //   timestamp is the timestamp the first voice frame received at.
  // NOTE:
  // If multiple speakers exist in the channel, there are multiple recording
  // file stored.
  // If |dir_prefix| is NULL or empty, the data file will be stored in the
  // current working directory.
  //
  // NOTE:
  // Agora transporter would use the ports from [minAllowedUdpPort,
  // maxAllowedUdpPort] for communicating with our signal servers,
  // voice servers, etc. This range should be equal to or larger than 5.
  //
  // If those arguments are given zero, Agora transporter will bind the
  // udp socket to the port allocated by the operating system.
  virtual void joinChannel(const char *vendorKey, const char *channelName,
      uint32_t uid, uint16_t minAllowedUdpPort=0,
      uint16_t maxAllowedUdpPort=0, const char *dir_prefix=NULL) = 0;

  // Preconditions:
  //  |joinChannel| is called and returned
  virtual void leave() = 0;

  // onTimer should be called every 10 milliseconds
  // In this function, voice data from the other side will be pulled from
  // voice Media SDK
  //
  // This function will
  //   1) If a join request is acknowledged, |onJoinSuccess| will be called
  //   2) If some errors occurred, |onError| will be called; otherwise
  // Preconditions:
  //  |joinChannel| is called and returned
  virtual void onTimer() = 0;
};

}
}

