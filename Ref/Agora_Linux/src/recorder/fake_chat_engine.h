#pragma once

// #include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "internal/chat_engine_i.h"
#include "recorder/chat_engine_observer.h"

namespace agora {
namespace recorder {
class active_stream;

struct parameter_value {
  enum value_type {kUnknown, kBool, kInt, kDouble, kString};
 public:
  explicit parameter_value(bool value=false);
  explicit parameter_value(int value);
  explicit parameter_value(double value);
  explicit parameter_value(const char *str);
  explicit parameter_value(const std::string &str);

  value_type get_type() const;
  bool get_as_bool() const;
  int get_as_int() const;
  double get_as_double() const;
  const std::string& get_as_string() const;
 private:
  value_type type_;

  // old-fashioned, for only C++ 11 supports members with non-trivial ctors
  // in union class.
  union {
    bool b_;
    int i_;
    double d_;
  };
  std::string str_;
};

class fake_chat_engine :public media::IChatEngine, private media::IAudioEngine {
 public:
  fake_chat_engine(chat_engine_observer *handler, const std::string &prefix);
  virtual ~fake_chat_engine();

  // The following virtual functions inherited from |IChatEngine|
  virtual void release();

  // Shared by |IChatEngine| and |IAudioEngine|
  virtual int init(media::IChatEngineObserver *observer);

  // Not used.
  virtual int initVideo();

  // inherited from both |IChatEngine| and |IAudioEngine|
  virtual int terminate();
  virtual int startCall();
  virtual int stopCall();
  virtual bool isCalling();

  // important
  virtual int setParameters(const char *parameters);
  virtual int getParameters(const char *parameters, char *buffer, size_t *length);
  virtual media::IAudioEngine* getAudioEngine();
  virtual media::IVideoEngine* getVideoEngine();

  // ?? trivial
  virtual int getEngineEvents(media::ChatEngineEventData &events);

  virtual int getParameter(const char* key, const char* args,
      media::util::AString& result);

  virtual int setBoolParameter(const char* key, bool value);
  virtual int setIntParameter(const char* key, int value);
  virtual int setNumberParameter(const char* key, double value);
  virtual int setStringParameter(const char* key, const char* value);
  virtual int setObjectParameter(const char* key, const char* value);
 private:
  // The following virtual functions inherited from |IAudioEngine|
  virtual int init();
  virtual int registerTransport(media::IAudioTransport *transport);
  virtual int deRegisterTransport();
  virtual int receiveNetPacket(unsigned int uid, const unsigned char *payload,
      unsigned short payloadSize, int payload_type, unsigned int timeStamp,
      unsigned short seqNumber);

  // ?? trivial
  virtual int registerNetEQStatistics(media::IJitterStatistics *neteq_statistics);
  virtual int deRegisterNetEQStatistics();

  // ?? trivial
  virtual int registerChatEngineObserver(media::IChatEngineObserver *observer);
  virtual int deRegisterChatEngineObserver();
  virtual bool isCallInterrupted();

  // Use wave file as input
  // NOTE: This function should not be called.
  virtual int startFileAsInput(const char *filename);

  // codec-setting functions
  // NOTE: Those functions should never be called, because I'm just a recorder
  virtual int numOfCodecs();
  virtual int getCodecInfo(int index, char *pInfoBuffer, int infoBufferLen);
  virtual int setCodec(int index);
  virtual int setMultiFrameInterleave(int num_frame, int num_interleave);
  virtual int setREDStatus(bool enable);
  virtual int setDTXStatus(DTX_MODE dtx_mode);

  // APM functions
  // FIXME(liuyong): Should I record down those settings?
  virtual int setAecType(AEC_TYPE type);
  virtual int setAudioOutputRouting(AUDIO_OUTPUT_ROUTING routing);
  virtual int setAgcStatus(bool enable);
  virtual int setNsStatus(bool enable);
  virtual int getJitterBufferMaxMetric();

  // Debug recording
  // NOTE: Those functions should never be called.
  virtual int startDebugRecording(const char *filename);
  virtual int stopDebugRecording();

  // Start recap playing
  // NOTE: Should never be called.
  virtual int startRecapPlaying();

  // Enable/Disable recap.
  // reportIntervalMs designates the callback interval in milliseconds,
  // 0 means disable
  // NOTE: Should never be called.
  virtual int setRecapStatus(int reportIntervalMs);

  // Set the NetEQ max and min playout delay.
  // NOTE: Record down these settings.
  virtual int setNetEQMaximumPlayoutDelay(int delayMs);
  virtual int setNetEQMinimumPlayoutDelay(int delayMs);

  virtual int setSpeakerStatus(bool enable);
  virtual int getPlayoutTS(unsigned int uid, int *playoutTS);

  // Enable/Disable volume/level report
  // reportIntervalMs: designates the callback interval in milliseconds,
  // 0 means disable
  //
  // smoothFactor: decides the smoothness of the returned volume, [0, 10],
  // default is 10
  // TODO(liuyong):  Ask for the function of this function.
  virtual int setSpeakersReportStatus(int reportIntervalMs, int smoothFactor);
  // Enable/Disable recording during the call
  // NOTE: Do not call the following functions.
  virtual int startCallRecording(const char *filename);
  virtual int stopCallRecording();
  virtual int startDecodeRecording(const char *filename);
  virtual int stopDecodeRecording();

  // Mute or Unmute
  virtual int setMuteStatus(bool enable);
 private:
  // NOTE(liuyong): All these following functions are unused.
  // Sets the type of audio device layer to use.
  // NOTE: unused
  virtual int setAudioDeviceLayer(AUDIO_LAYERS audioLayer);

  // Gets the number of audio devices available for recording.
  // NOTE: unused
  virtual int getNumOfRecordingDevices(int &devices);

  // Gets the number of audio devices available for playout.
  // NOTE: unused
  virtual int getNumOfPlayoutDevices(int &devices);

  // Gets the name of a specific recording device given by an |index|.
  // On Windows Vista/7, it also retrieves an additional unique ID
  // (GUID) for the recording device.
  // NOTE: Unused
  virtual int getRecordingDeviceName(int index, char name[128],
                                     char deviceId[128]);

  // Gets the name of a specific playout device given by an |index|.
  // On Windows Vista/7, it also retrieves an additional unique ID
  // (GUID) for the playout device.
  // NOTE: Unused
  virtual int getPlayoutDeviceName(int index, char name[128],
                                   char deviceId[128]);

  // Checks if the sound card is available to be opened for recording.
  // NOTE: Unused
  virtual int getRecordingDeviceStatus(bool &isAvailable);

  // Checks if the sound card is available to be opened for playout.
  // NOTE: Unused
  virtual int getPlayoutDeviceStatus(bool &isAvailable);

  // Sets the audio device used for recording.
  // NOTE: Unused
  virtual int setRecordingDevice(int index);
  virtual int setRecordingDevice(const char deviceId[128]);

  // Sets the audio device used for playout.
  virtual int setPlayoutDevice(int index);
  virtual int setPlayoutDevice(const char deviceId[128]);

  // Gets current selected audio device used for recording.
  virtual int getCurrentRecordingDevice(char deviceId[128]);

  // Gets current selected audio device used for playout.
  virtual int getCurrentPlayoutDevice(char deviceId[128]);
 private:
  bool load_config();
  void clear_streams();
 private:
  chat_engine_observer *observer_;

  bool is_calling_;
  media::IAudioTransport *transport_;

  bool record_packet_;
  std::string prefix_dir_;
  std::string channel_name_;
  std::mutex stream_lock_;

  // unique_ptr is not introduced until C++ 0x
  std::unordered_map<uint32_t, active_stream *> streams_;

  // storing parameters
  std::unordered_map<std::string, parameter_value> parameters_;
};

}
}

