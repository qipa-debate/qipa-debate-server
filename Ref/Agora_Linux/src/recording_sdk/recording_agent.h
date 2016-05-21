#pragma once

#include <cstdint>
#include <string>
#include "include/recording_sdk.h"
#include "base/app_controller.h"
#include "base/atomic.h"

namespace agora {
namespace recorder {
class RecordStub : public IRecordSdk {
 public:
  explicit RecordStub(IRecordEventHandler *callback);
  virtual ~RecordStub();

  virtual void joinChannel(const char *vendorKey,
      const char *channelName, uint32_t uid, uint16_t minPort=0,
      uint16_t maxPort=0, const char *dir_prefix=NULL);
  virtual void leave();
  virtual void onTimer();
 private:
  bool handle_join_reply(uint32_t cid, uint32_t uid, int code,
      const std::string &info);

  bool handle_voice_data(unsigned int uid, const std::string &data);
  bool handle_recorder_error(int code, const std::string &info);
  bool handle_leave_reply();
  bool handle_session_creation(const std::string &sessionId);
  bool handle_speaker_info(uint32_t uid, const std::string &path);
  bool handle_channel_idle(int32_t idle);
 private:
  atomic_bool_t started_;
  IRecordEventHandler *callback_;
  base::app_controller controller_;
  std::string channel_name_;
};
}
}

