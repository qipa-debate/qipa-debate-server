#include "recorder/active_stream.h"
#include "base/safe_log.h"
#include "internal/recap_format.h"
#include <sys/time.h>
#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <ctime>
#include <memory>

using namespace std;

namespace agora {
namespace recorder {
int64_t now_mseconds() {
  timeval t;
  if (0 != gettimeofday(&t, NULL)) {
    return -1;
  }

  return t.tv_sec * 1000 + t.tv_usec / 1000;
}

active_stream::active_stream(const string &prefix, const std::string &cname,
    uint32_t uid) {
  prefix_ = prefix;
  channel_name_ = cname;
  uid_ = uid;
  fp_ = NULL;
}

active_stream::~active_stream() {
  close();
}

bool active_stream::open() {
  start_ms_ = now_mseconds();

  char buf[512];
  if (!prefix_.empty()) {
    snprintf(buf, 511, "%s/%s-%" PRIu32 "-%" PRIu32 ".agr",
        prefix_.c_str(), channel_name_.c_str(), uid_,
        static_cast<uint32_t>(start_ms_ / 1000));
  } else {
    snprintf(buf, 511, "%s-%" PRIu32 "-%" PRIu32 ".agr",
        channel_name_.c_str(), uid_, static_cast<uint32_t>(start_ms_ / 1000));
  }

  if ((fp_ = fopen(buf, "wb")) == NULL) {
    SAFE_LOG(ERROR) << "Failed to open " << buf
        << ", Reason: " << strerror(errno);
    return false;
  }

  file_name_ = buf;
  return write_header();
}

bool active_stream::write_header() {
  int64_t now = now_mseconds();
  if (now < 0)
    return false;

  recorder_file_header2 header;
  header.magic_id = AGORA_MAGIC_ID;
  header.version = {2, 0};
  header.data_offset = sizeof(header);
  header.uid = uid_;
  header.start_ms = now;

  const size_t n = (std::min)(channel_name_.size(), static_cast<size_t>(63));
  strncpy(header.channel_name, &channel_name_[0], n);
  header.channel_name[n] = '\0';

  return fwrite(&header, sizeof(header), 1, fp_) == 1;
}

bool active_stream::record_net_packet(
    const unsigned char *payload,
    unsigned short payload_size,
    int payload_type,
    unsigned int timestamp,
    unsigned short seq_number) {
  if (!fp_) {
    SAFE_LOG(WARN) << "File not opened yet, a packet dropped: "
        << channel_name_ << ", " << uid_;
    return false;
  }

  int packet_size = payload_size + sizeof(audio_packet);
  std::unique_ptr<char[]> buf(new char[packet_size]);

  audio_packet &p = reinterpret_cast<audio_packet &>(*buf.get());
  p.ts_offset = static_cast<int32_t>(now_mseconds() - start_ms_);
  p.payload_size = payload_size;
  p.payload_type = payload_type;
  p.timestamp = timestamp;
  p.seq_number = seq_number;
  memcpy(&p + 1, payload, payload_size);

  if (1 != fwrite(buf.get(), packet_size, 1, fp_)) {
    return false;
  }

  return true;
}

bool active_stream::flush() {
  if (fp_) {
    fflush(fp_);
    return true;
  }

  return false;
}

bool active_stream::close() {
  if (fp_ && fclose(fp_) != 0) {
    SAFE_LOG(ERROR) << "Failed to close the file: " << channel_name_
        << ", " << uid_;
    return false;
  }

  return true;
}

}
}

