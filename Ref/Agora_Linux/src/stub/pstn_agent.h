#pragma once

#include <cstdint>
#include <string>
#include "include/agora_sdk_i.h"
#include "base/app_controller.h"
#include "base/atomic.h"

namespace agora {
namespace pstn {
class PstnStub : public IAgoraSdk {
 public:
  explicit PstnStub(IAgoraSdkEventHandler *callback);
  virtual ~PstnStub();

  virtual void joinChannel(const char *vendorID, const char *vendorKey,
      const char *channelName, uint32_t uid, uint16_t minPort=0,
      uint16_t maxPort=0, bool mixed=true, int32_t sampleRates=8000,
      uint32_t reserved=0, int linger_time=-1);

  virtual void leave();
  virtual void sendVoiceData(const char *pbuffer, uint32_t length);
  virtual void onTimer();
  virtual void notifyPhoneEvent(PhoneEvent status);
 private:
  bool handle_join_reply(uint32_t cid, uint32_t uid, int code,
      const std::string &info);

  bool handle_voice_data(unsigned int uid, const std::string &data);
  bool handle_applite_error(int code, const std::string &info);
  bool handle_leave_reply();
  bool handle_session_creation(const std::string &sessionId);
  bool handle_speaker_info(uint32_t uid, const std::string &path);
  bool handle_channel_idle(int32_t idle);
  bool handle_user_joined(uint32_t uid, int elapsed);
  bool handle_user_offline(uint32_t uid, int reason);
 private:
  std::string channel_name_;
  atomic_bool_t started_;
  IAgoraSdkEventHandler *callback_;
  base::app_controller controller_;
  std::string cname_;
};
}
}

