#pragma once

#include <cstdint>

#include "internal/chat_engine_server_i.h"

namespace agora {
namespace player {

class engine_driver : private media::IAudioTransport,
    private media::ITraceCallback {
 public:
  explicit engine_driver(int32_t samples=8000);
  virtual ~engine_driver();

  bool play_file(const char *filename, const char *output, int speed);
 private:
  bool init_engine();
  bool terminate_engine();
  bool record_pcm(const char *output, int speed);
 private:
  virtual int sendAudioPacket(const unsigned char *payloadData,
      unsigned short payloadSize, int payload_type, int vad_type,
      unsigned int timeStamp, unsigned short seqNumber);

  virtual void Print(int level, const char *message, int length);
 private:
  media::IChatEngine *engine_;
  media::ITraceService *trace_;

  const int32_t sample_rates_;

  static const unsigned char kBytesPerSample = 2;
  static const unsigned char kChannels = 1;
  static const unsigned int kMaxStreams = 6;
};

}
}

