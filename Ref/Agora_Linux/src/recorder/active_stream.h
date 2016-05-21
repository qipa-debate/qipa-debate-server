#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

namespace agora {
namespace recorder {
class active_stream {
 public:
  active_stream(const std::string &prefix, const std::string &cname,
      uint32_t uid);
  ~active_stream();

  bool open();

  bool record_net_packet(const unsigned char *payload,
      unsigned short payload_size,
      int payload_type,
      unsigned int timestamp,
      unsigned short seq_number);

  const std::string& get_recorder_file() const { return file_name_; }
 private:
  bool flush();
  bool close();
  bool write_header();
 private:
  std::string prefix_;
  std::string file_name_;
  std::string channel_name_;

  uint32_t uid_;
  int64_t start_ms_;

  FILE *fp_;
};
}
}

