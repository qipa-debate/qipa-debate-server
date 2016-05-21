#include "applite/audio_recorder.h"

#include "base/safe_log.h"

#include "applite/pcm_recorder.h"
#include "applite/mp3_recorder.h"

namespace agora {
namespace pstn {

audio_recorder::~audio_recorder() {
}

audio_recorder* audio_recorder::create(const char *prefix, int channels,
    int sample_rates, file_format fmt, int slice) {
  if (fmt == kRawPCM) {
    return new (std::nothrow)pcm_recorder(prefix, slice);
  } else if (fmt == kMp3) {
    return new (std::nothrow)mp3_recorder(prefix, channels,
        sample_rates, slice);
  }

  SAFE_LOG(ERROR) << "Unrecognized file format: " << fmt;
  return NULL;
}

}
}

