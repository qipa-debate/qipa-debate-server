#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "internal/rtc/IAgoraRtcEngine.h"
#include "internal/rtc/rtc_engine_i.h"
#include "internal/fake_audio_device.h"

#include "base/atomic.h"
#include "base/packet_pipe.h"

#include "applite/audio_recorder.h"

namespace agora {
namespace pstn {
class pcm_recorder;
class speaker;

class audio_event_handler : private rtc::IRtcEngineEventHandlerEx,
    private media::IAudioFrameObserver {
 public:
  audio_event_handler(int read_fd, int write_fd, uint32_t uid,
      const std::string &vendor_key, const std::string &channel_name,
      bool mixed=true, int32_t samples=8000, int idle=120,
      bool batch_mode=false, const char *path_prefix="", bool pstn=false,
      int slice = 0x7FFFFFFF, const char *codec="pcm");

  ~audio_event_handler();

  int run();
 private:
  int run_interactive(int64_t join_start_ts);
  int run_in_batch(int64_t join_start_ts);
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

  // inherited from IAudioFrameObserver
  virtual bool onRecordFrame(void *audioSample, int nSamples,
      int nBytesPerSample, int nChannels, int sampleRates);

  virtual bool onPlaybackFrame(void *audioSample, int nSamples,
      int nBytesPerSample, int nChannels, int sampleRates);

  virtual bool onPlaybackFrameUid(unsigned int uid, void *audioSample,
      int nSamples, int nBytesPerSample, int nChannels, int sampleRates);
 private:
  void cleanup();
  bool is_solo_channel() const;

  // Receives messages from the PSTN
  void on_voice_data(const std::string &data);
  void on_leave();
  void on_phone_event(int status);
  void on_pickup_phone();

  void handle_request(base::unpacker &pk, uint16_t uri);

  // Notify the PSTN of new messages
  bool fetch_voice_data();

  bool send_voice_data(uint32_t uid, const char *buf, int len);
  bool send_join_reply(const char *cid, uint32_t uid);
  bool send_error_report(int rescode, const char *msg);
  bool send_leave_reply();
  bool send_session_creation_event(const char *sessionId);
  bool send_idle_info();
  int send_pending_packets();

  bool send_user_join(uid_t uid, int elapsed);
  bool send_user_offline(uid_t uid, int reason);

  bool get_recording_path(unsigned int uid, int64_t ts, bool down,
      std::string *path) const;

  static void term_handler(int sig_no);
  static atomic_bool_t s_broken_pipe_;
 private:
  const int read_fd_;
  const int write_fd_;
  const uint32_t uid_;
  const std::string vendor_key_;
  const std::string channel_name_;
  const bool audio_mixed_;
  const bool is_batch_mode_;
  const int32_t sample_rates_;
  const int channel_idle_;
  const bool is_pstn_;
  const int slice_;
  const file_format stored_fmt_;

  atomic_bool_t joined_;
  int32_t last_active_ts_;
  unsigned int max_users_;

  std::mutex in_mutex_;
  std::deque<char> in_data_;

  // for recording upstream/down pcm data.
  pcm_recorder *up_pcm_recorder_;
  pcm_recorder *down_pcm_recorder_;

  base::pipe_reader *reader_;
  base::pipe_writer *writer_;
  rtc::IRtcEngineEx *applite_;

  std::string path_prefix_;

  std::mutex speaker_mutex_;
  std::unordered_map<unsigned, speaker *> speakers_;

  typedef std::vector<char> packet_buffer_t;
  std::queue<packet_buffer_t> pending_packets_;

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

