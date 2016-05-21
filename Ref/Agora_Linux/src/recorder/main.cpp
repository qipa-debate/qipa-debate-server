// #include <atomic>
#include <cstdint>
#include <cinttypes>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "base/log.h"
#include "base/opt_parser.h"
#include "base/packet_pipe.h"
#include "recorder/audio_event_handler.h"

#include "base/atomic.h"

using namespace std;
using namespace agora;
using namespace agora::base;
using namespace agora::recorder;

audio_event_handler *g_audio_handler = NULL;
std::string g_recording_dir;
std::string g_channel_name;

int main(int argc, char *argv[]) {
  int32_t min_port = 0;
  int32_t max_port = 0;
  int32_t mixed = 1; // true
  int32_t samples = 8000;
  int read_fd = -1;
  int write_fd = -1;
  uint32_t uid = 0;
  int idle = 12000;
  string key;
  bool batch = false;

  LOG(INFO, "Agora recorder, version " GIT_DESC);

  opt_parser parser;
  parser.add_long_opt("min_port", &min_port);
  parser.add_long_opt("max_port", &max_port);
  parser.add_long_opt("read", &read_fd);
  parser.add_long_opt("write", &write_fd);
  parser.add_long_opt("uid", &uid);
  parser.add_long_opt("key", &key);
  parser.add_long_opt("name", &g_channel_name);
  parser.add_long_opt("audio_mix", &mixed);
  parser.add_long_opt("samples", &samples);
  parser.add_long_opt("idle", &idle);
  parser.add_long_opt("dir", &g_recording_dir);
  parser.add_long_opt("batch", &batch);

  if (!parser.parse_opts(argc, argv) || (samples != 8000 && samples != 16000)) {
    parser.print_usage(argv[0], cerr);
    return -1;
  }

  idle = (std::max)(idle, 60);

  if (batch) {
    if (read_fd >= 0 || write_fd >= 0) {
      cerr << "No read_fd/write_fd should be set under batch mode\n";
      return -2;
    }
  } else {
    if (read_fd < 0 || write_fd < 0) {
      cerr << "No read_fd/write_fd is given under programmed mode\n";
      return -2;
    }
  }

  LOG(INFO, "uid %" PRIu32 " from vendor %s is joining channel %s", uid,
      key.c_str(), g_channel_name.c_str());

  g_audio_handler = new (std::nothrow) audio_event_handler(read_fd, write_fd,
      uid, key, g_channel_name, mixed, samples, idle);

  int result = g_audio_handler->run();

  delete g_audio_handler;
  return result;
}

