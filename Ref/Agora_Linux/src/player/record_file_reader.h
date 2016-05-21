#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

namespace agora {
namespace recorder {
struct audio_packet;
}

namespace player {
class record_file_reader {
 public:
  record_file_reader();
  ~record_file_reader();

  bool open(const char *file);
  bool get_next_packet(recorder::audio_packet *ppacket, void **buf);

  const std::string& get_cname() const;
  uint32_t get_cid() const;
  uint32_t get_uid() const;
  int64_t get_start_ms() const;
 private:
  FILE *fp_;

  bool is_newer_version_;

  std::string cname_;
  uint32_t cid_;
  uint32_t uid_;
  int64_t start_ms_;
};

inline uint32_t record_file_reader::get_cid() const {
  return cid_;
}

inline const std::string& record_file_reader::get_cname() const {
  return cname_;
}

inline uint32_t record_file_reader::get_uid() const {
  return uid_;
}

inline int64_t record_file_reader::get_start_ms() const {
  return start_ms_;
}

}
}

