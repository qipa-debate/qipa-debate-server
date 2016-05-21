#pragma once

#include <cstdlib>

namespace agora {
namespace pstn {

enum file_format {kRawPCM = 0, kMp3 = 1};

class audio_recorder {
 public:
  static audio_recorder* create(const char *file_prefix, int channels,
      int sample_rates, file_format fmt = kRawPCM, int slices = 0x7FFFFFFF);

  virtual ~audio_recorder() = 0;

  virtual bool initialize() = 0;
  virtual bool finalize() = 0;
  virtual bool write_audio(const void *buf, size_t length) = 0;
};

}
}

