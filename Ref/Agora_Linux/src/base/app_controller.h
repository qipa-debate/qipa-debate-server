#pragma once

#include <cstdint>

#include "base/packet_pipe.h"

namespace agora {
namespace base {
class unpacker;
class pipe_reader;
class pipe_writer;

class app_controller {
 public:
  explicit app_controller(const char *applite="applite");
  ~app_controller();

  bool create_app(const char *vendor_key, const char *channel_name,
      uint32_t uid, uint16_t min_port, uint16_t max_port, bool mixed=true,
      int32_t sampleRates=8000, const char *dir_prefix=NULL,
      uint32_t reserved=0, int linger_time=-1);

  bool good() const;
  void close_controller();

  bool send_voice_data(const char *buf, uint32_t len);
  base::ReadState read_packet(base::unpacker *p, uint16_t *uri);
  bool leave_channel();
  bool notify_phone_event(int status);
 private:
  const char *applite_;
  base::pipe_reader *reader_;
  base::pipe_writer *writer_;
 private:
  app_controller(const app_controller &) = delete;
  app_controller(app_controller &&) = delete;
  app_controller& operator=(const app_controller &) = delete;
  app_controller& operator=(app_controller &&) = delete;
};

}
}

