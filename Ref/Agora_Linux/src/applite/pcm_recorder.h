#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "applite/audio_recorder.h"

namespace agora {
namespace pstn {

class pcm_recorder : public audio_recorder {
 public:
  explicit pcm_recorder(const char *prefix, int slice=10000000);
  virtual ~pcm_recorder();

  virtual bool initialize();
  virtual bool finalize();
  virtual bool write_audio(const void *buf, size_t length);
 private:
  bool create_pcm_file();
  bool flush_file();
 private:
  std::string prefix_;
  const int slice_;

  int cur_slice_;
  int32_t begin_ts_;

  FILE *pcm_file_;
  void *buf_;
};

}
}

