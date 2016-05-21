#include "applite/mp3_recorder.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "base/safe_log.h"
#include "base/util.h"

using namespace std;

namespace agora {
namespace pstn {

mp3_recorder::mp3_recorder(const char *prefix, int channels, int sample_rates,
    int slice) :prefix_(prefix), channels_(channels),
    sample_rates_(sample_rates), slice_(slice) {
  cur_slice_ = 0;
  begin_ts_ = 0;
  mp3_encoder_ = NULL;
  mp3_file_ = NULL;
}

mp3_recorder::~mp3_recorder() {
  if (mp3_file_) {
    fclose(mp3_file_);
  }

  if (mp3_encoder_) {
    lame_close(mp3_encoder_);
  }
}

bool mp3_recorder::initialize() {
  if ((mp3_encoder_ = lame_init()) == NULL) {
    SAFE_LOG(FATAL) << "Unable to create the mp3 encoder";
    return false;
  }

  lame_set_brate(mp3_encoder_, 64);
  lame_set_num_channels(mp3_encoder_, channels_);
  lame_set_in_samplerate(mp3_encoder_, sample_rates_);
  lame_set_out_samplerate(mp3_encoder_, sample_rates_);

  lame_set_mode(mp3_encoder_, MONO);

  lame_init_params(mp3_encoder_);


  cur_slice_ = 0;
  begin_ts_ = base::now_ts();

  if (!create_mp3_file()) {
    return false;
  }

  return true;
}

bool mp3_recorder::create_mp3_file() {
  ostringstream sout;
  sout << prefix_ << "_" << cur_slice_ << ".mp3";

  string filename = sout.str();
  if ((mp3_file_ = fopen(filename.c_str(), "wb")) == NULL) {
    SAFE_LOG(FATAL) << "Failed to open the MP3 file " << filename
        << " to write, reason: " << strerror(errno);
    return false;
  }

  return true;
}

bool mp3_recorder::flush_file() {
  if (mp3_file_) {
    unsigned char encoded[8 * 1024];
    assert(mp3_encoder_ != NULL);

    int len = lame_encode_flush_nogap(mp3_encoder_, encoded, sizeof(encoded));
    if (len > 0)
      fwrite(encoded, 1, len, mp3_file_);
    //lame_mp3_tags_fid((lame_global_flags *)mp3_encoder, mp3_file_);
    fclose(mp3_file_);

    ++cur_slice_;
    mp3_file_ = NULL;
  }

  return true;
}

bool mp3_recorder::finalize() {
  flush_file();

  if (mp3_encoder_) {
    lame_close(mp3_encoder_);
    mp3_encoder_ = NULL;
  }

  return true;
}

bool mp3_recorder::write_audio(const void *buf, size_t length) {
  time_t now = time(NULL);
  if (now - begin_ts_ > slice_) {
    flush_file();
    if (!create_mp3_file())
      return false;
    begin_ts_ = now;
  }

  return write_data(buf, length);
}

bool mp3_recorder::write_data(const void *buf, size_t length) {
  if (!mp3_file_)
    return false;

  // NOTE(liuyong): improve this code
  int samples = length >> 1;
  int size = 8000 + ((5 * samples) >> 2);
  const short *buffer = reinterpret_cast<const short *>(buf);

  while (true) {
    size += samples;
    vector<unsigned char> encoded(size);
    int result = lame_encode_buffer(mp3_encoder_, buffer, buffer, samples,
        &encoded[0], size);
    if (result == -1)
      continue;

    if (result >= 0) {
      fwrite(&encoded[0], 1, result, mp3_file_);
    } else {
      SAFE_LOG(ERROR) << "Error in write_data: " << result;
      return false;
    }
    break;
  }

  return true;
}

}
}

