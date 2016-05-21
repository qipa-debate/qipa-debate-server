#include <unistd.h>
#include <cinttypes>

#include "base/log.h"
#include "base/process.h"
#include "protocol/app_comm.h"
#include "base/app_controller.h"

using std::string;

namespace agora {
using namespace pstn;

namespace base {

app_controller::app_controller(const char *applite) {
  applite_ = applite;

  reader_ = NULL;
  writer_ = NULL;
}

app_controller::~app_controller() {
  delete reader_;
  delete writer_;
}

bool app_controller::create_app(const char *vendor_key,
    const char *channel_name, uint32_t uid, uint16_t min_port,
    uint16_t max_port, bool mixed, int32_t sampleRates,
    const char *dir_prefix, uint32_t reserved, int linger_time) {
  int fds1[2];
  int fds2[2];
  if (pipe(fds1) != 0) {
    LOG(ERROR, "Failed to create pipes: %m");
    return false;
  }

  if (pipe(fds2) != 0) {
    LOG(ERROR, "Failed to create pipes: %m");
    close(fds1[0]);
    close(fds1[1]);

    return false;
  }

  reader_ = new pipe_reader(fds1[0]);
  writer_ = new pipe_writer(fds2[1]);

  char min_port_str[16];
  snprintf(min_port_str, 16, "%d", min_port);
  char max_port_str[16];
  snprintf(max_port_str, 16, "%d", max_port);

  char uid_str[16];
  snprintf(uid_str, 16, "%u", uid);

  char pipe_in_str[16];
  snprintf(pipe_in_str, 16, "%d", fds2[0]);

  char pipe_out_str[16];
  snprintf(pipe_out_str, 16, "%d", fds1[1]);

  char mixed_str[4];
  snprintf(mixed_str, 4, "%d", mixed ? 1 : 0);

  char samples[16];
  snprintf(samples, 16, "%d", sampleRates);

  char reserved_str[16];
  snprintf(reserved_str, 16, "%" PRIu32, reserved);

  const char *args[] = {applite_, "--min_port", min_port_str,
    "--max_port", max_port_str, "--read", pipe_in_str, "--write", pipe_out_str,
    "--uid", uid_str, "--key", vendor_key, "--name", channel_name,
    "--audio_mix", mixed_str, "--samples", samples, "--reserved", reserved_str,
    NULL, NULL, NULL, NULL, NULL};
  static const int kArgLen = sizeof(args) / sizeof(args[0]);
  int pos = kArgLen - 5;

  if (dir_prefix != NULL && *dir_prefix != '\0') {
    args[pos++] = "--dir";
    args[pos++] = dir_prefix;
  }

  if (linger_time > 10) {
    args[pos++] = "--idle";
    char linger_time_str[16];
    snprintf(linger_time_str, 16, "%d", linger_time);
    args[pos++] = linger_time_str;
  }

  int skipped_fds[] = {0, 1, 2, fds2[0], fds1[1]};
  int len = sizeof(skipped_fds) / sizeof(int);

  if (!base::create_process(args, false, false, NULL, skipped_fds, len)) {
    LOG(ERROR, "Failed to spawn an applite: %m");
    // close all the remained fds
    close(fds1[1]);
    close(fds2[0]);

    delete reader_;
    delete writer_;

    reader_ = NULL;
    writer_ = NULL;

    return false;
  }

  // close the unnecessary pipe fds
  close(fds1[1]); // the first pipe aimed for read only
  close(fds2[0]); // the second pipe aimed for write only

  return true;
}

bool app_controller::send_voice_data(const char *buf, uint32_t len) {
  assert(writer_ != NULL);

  pstn::p_voice_data data;
  data.data = string(buf, len);
  return writer_->write_packet(data) == base::kWriteOk;
}

bool app_controller::leave_channel() {
  int from = 0; // unused.
  pstn::p_leave_request request;
  request.from = from;
  if (writer_->write_packet(request) != base::kWriteOk) {
    return false;
  }

  // wait until the applite returns
  int retry = 0;
  while (++retry < 5) {
    unpacker pk;
    uint16_t uri = EMPTY_URI;

    base::ReadState state = read_packet(&pk, &uri);
    if (state == base::kEof || state == base::kError || (state == base::kReceived &&
        uri == LEAVE_REPLY_URI)) {
      break;
    }

    sleep(1);
  }

  if (retry == 5)
    return false;
  return true;
}

base::ReadState app_controller::read_packet(unpacker *p, uint16_t *uri) {
  assert(reader_ != NULL);
  return reader_->read_packet(p, uri);
}

bool app_controller::notify_phone_event(int status) {
  p_phone_event notification;
  notification.status = status;
  return writer_->write_packet(notification) == base::kWriteOk;
}

bool app_controller::good() const {
  if (!reader_ || !writer_)
    return false;

  return reader_->is_good() && writer_->is_good();
}

void app_controller::close_controller() {
  if (reader_) {
    delete reader_;
    reader_ = NULL;
  }

  if (writer_) {
    delete writer_;
    writer_ = NULL;
  }
}

}
}

