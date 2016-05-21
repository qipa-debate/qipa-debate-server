#include "applite/pcm_recorder.h"

#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "base/safe_log.h"
#include "base/util.h"

using namespace std;

namespace agora {
namespace pstn {

pcm_recorder::pcm_recorder(const char *prefix, int slice) :prefix_(prefix),
    slice_(slice) {
  cur_slice_ = 0;
  begin_ts_ = 0;

  pcm_file_ = NULL;
  buf_ = NULL;
}

pcm_recorder::~pcm_recorder() {
  if (pcm_file_)
    fclose(pcm_file_);

  free(buf_);
}

bool pcm_recorder::initialize() {
  cur_slice_ = 0;
  begin_ts_ = base::now_ts();

  if (!create_pcm_file()) {
    return false;
  }

  return true;
}

bool pcm_recorder::finalize() {
  if (pcm_file_) {
    fflush(pcm_file_);
    fclose(pcm_file_);
    pcm_file_ = NULL;
  }

  free(buf_);
  buf_ = NULL;

  return true;
}

bool pcm_recorder::flush_file() {
  if (pcm_file_) {
    fflush(pcm_file_);
    fclose(pcm_file_);

    ++cur_slice_;
    pcm_file_ = NULL;
  }

  return true;
}

bool pcm_recorder::create_pcm_file() {
  ostringstream sout;
  sout << prefix_ << "_" << cur_slice_ << ".pcm";

  string filename = sout.str();
  if ((pcm_file_ = fopen(filename.c_str(), "wb")) == NULL) {
    SAFE_LOG(FATAL) << "Failed to open the PCM file " << filename
        << " to write, reason: " << strerror(errno);
    return false;
  }

  int page_size = getpagesize();
  int size = page_size * 128;
  int ret = posix_memalign(&buf_, page_size, size);
  if (ret != 0 || buf_ == NULL) {
    SAFE_LOG(ERROR) << "Failed to allocate paged memory: " << ret << buf_;
    return false;
  }

  return setvbuf(pcm_file_, reinterpret_cast<char *>(buf_), _IOFBF, size) == 0;
}

bool pcm_recorder::write_audio(const void *buf, size_t len) {
  time_t now = time(NULL);
  if (now - begin_ts_ > slice_) {
    flush_file();
    if (!create_pcm_file())
      return false;
    begin_ts_ = now;
  }

  size_t count = len >> 1;
  return count == fwrite(buf, 2, count, pcm_file_);
}

}
}

