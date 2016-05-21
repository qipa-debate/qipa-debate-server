#pragma once

#include <cstdio>
#include <string>

#include "third_party/lame/lame.h"
#include "applite/audio_recorder.h"

namespace agora {
namespace pstn {

class mp3_recorder : public audio_recorder {
 public:
  mp3_recorder(const char *prefix, int channels, int sample_rates,
      int slice=10000000);

  virtual ~mp3_recorder();

  virtual bool initialize();
  virtual bool finalize();
  virtual bool write_audio(const void *buf, size_t length);
 private:
  bool create_mp3_file();
  bool flush_file();
  bool write_data(const void *buf, size_t length);
 private:
  mp3_recorder(const mp3_recorder &) = delete;
  mp3_recorder& operator=(const mp3_recorder &) = delete;
  mp3_recorder(const mp3_recorder &&) = delete;
  mp3_recorder& operator=(const mp3_recorder &&) = delete;
 private:
  const std::string prefix_;
  const int channels_;
  const int sample_rates_;
  const int slice_;

  int cur_slice_;
  int32_t begin_ts_;

  lame_global_flags *mp3_encoder_;
  FILE *mp3_file_;
};

}
}

