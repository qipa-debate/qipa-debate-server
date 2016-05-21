#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/atomic.h"
#include "base/packet_pipe.h"

#include "internal/rtc/IAgoraRtcEngine.h"
#include "internal/rtc/rtc_engine_i.h"

#include "recorder/chat_engine_observer.h"

namespace agora {
namespace recorder {
class pcm_recorder;
class speaker;

class audio_event_handler :private rtc::IRtcEngineEventHandlerEx,
    public chat_engine_observer {
  enum {PAYLOAD_DEFAULT = 0, VAD_DEFAULT = 1, PAYLOAD_NO_STEP = 160};
 public:
  audio_event_handler(int read_fd, int write_fd, uint32_t uid,
      const std::string &vendor_key, const std::string &channel_name,
      bool mixed=true, int32_t samples=8000, int32_t idle=120);

  virtual ~audio_event_handler();

  int run();
 public:
  virtual void onSpeakerJoined(uint32_t uid, const std::string &path);
 private:
  virtual void onJoinChannelSuccess(const char *cid, uid_t uid, int elapsed);
  virtual void onRejoinChannelSuccess(const char *cid, uid_t uid, int elapsed);
  virtual void onWarning(int warn, const char *msg);
  virtual void onError(int err, const char *msg);
  virtual void onUserJoined(uid_t uid, int elapsed);
	virtual void onUserOffline(uid_t uid, rtc::USER_OFFLINE_REASON_TYPE reason);
  virtual void onRtcStats(const rtc::RtcStats &stats);

  // inherited from IRtcEngineEventHandlerEx
  virtual void onLogEvent(int level, const char *msg, int length);
 private:
  int run_interactive(int64_t join_start_ts);

  void cleanup();
  bool is_solo_channel() const;

  // Receives messages from the PSTN
  void on_voice_data(const std::string &data);
  void on_leave();
  void on_phone_event(int status);
  void on_pickup_phone();

  void handle_request(base::unpacker &pk, uint16_t uri);

  bool send_join_reply(const char *cname, uint32_t uid);
  bool send_error_report(int rescode, const char *msg);
  bool send_leave_reply();
  bool send_session_creation_event(const char *sessionId);
  bool send_idle_info();

  static void broken_pipe_handler(int sig_no);
  static atomic_bool_t s_broken_pipe_;
 private:
  const int read_fd_;
  const int write_fd_;
  const uint32_t uid_;
  const std::string vendor_key_;
  const std::string channel_name_;
  const bool audio_mixed_;
  const int32_t sample_rates_;
  const int channel_idle_;

  atomic_bool_t joined_;
  int32_t last_active_ts_;

  base::pipe_reader *reader_;
  base::pipe_writer *writer_;
  rtc::IRtcEngineEx *applite_;

  std::string path_prefix_;

  std::mutex speaker_mutex_;
  std::unordered_map<unsigned, speaker *> speakers_;

  static const unsigned char kBytesPerSample = 2;
  static const unsigned char kChannels = 1;
 private:
  audio_event_handler(const audio_event_handler &) = delete;
  audio_event_handler(audio_event_handler &&) = delete;
  audio_event_handler& operator=(const audio_event_handler &) = delete;
  audio_event_handler& operator=(audio_event_handler &&) = delete;
};

}
}

