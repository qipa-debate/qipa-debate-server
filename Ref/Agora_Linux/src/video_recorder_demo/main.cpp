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
#include "video_recorder_demo/audio_event_handler.h"

#include "base/atomic.h"

using namespace std;
using namespace agora;
using namespace agora::base;
using namespace agora::pstn;

bool g_record_pcm = false;
int g_recorded_time_span = 3600; // for an hour

std::string g_record_directory;

void load_config(const char *file) {
  ifstream fin(file);
  std::string line;
  while (getline(fin, line)) {
    string opt;
    string arg;
    istringstream sin(line);
    sin >> opt >> arg;
    if (opt.empty() || arg.empty())
      continue;
    if (opt == "record") {
      if (arg == "on")
        g_record_pcm = true;
    } else if (opt == "timespan") {
      g_recorded_time_span = atoi(opt.c_str());
      if (g_recorded_time_span < 600)
        g_recorded_time_span = 600;
    } else if (opt == "directory") {
      g_record_directory = arg;
    }
  }
}

int main(int argc, char *argv[]) {
  int32_t min_port = 0;
  int32_t max_port = 0;
  int32_t mixed = 1; // true
  int32_t samples = 8000;
  int read_fd = -1;
  int write_fd = -1;
  uint32_t uid = 0;
  int idle = 120000000;
  string key;
  string name;
  string path;
  bool batch = false;
  uint32_t reserved = 0;

  LOG(INFO, "Applite, based on version " GIT_DESC);

  opt_parser parser;
  parser.add_long_opt("min_port", &min_port);
  parser.add_long_opt("max_port", &max_port);
  parser.add_long_opt("read", &read_fd);
  parser.add_long_opt("write", &write_fd);
  parser.add_long_opt("uid", &uid);
  parser.add_long_opt("key", &key);
  parser.add_long_opt("name", &name);
  parser.add_long_opt("audio_mix", &mixed);
  parser.add_long_opt("samples", &samples);
  parser.add_long_opt("idle", &idle);
  parser.add_long_opt("path", &path);
  parser.add_long_opt("batch", &batch);
  parser.add_long_opt("reserved", &reserved);

  if (!parser.parse_opts(argc, argv) || (samples != 8000 && samples != 16000)) {
    parser.print_usage(argv[0], cout);
    return -1;
  }

  idle = (std::max)(idle, 20);

  LOG(INFO, "uid %" PRIu32 " from vendor %s is joining channel %s, mode: %s",
      uid, key.c_str(), name.c_str(), batch ? "batch" : "programmed");

  if (!batch && (read_fd <= 0 || write_fd <= 0)) {
    parser.print_usage(argv[0], cerr);
    return -1;
  }

  if (!batch) {
    load_config("/etc/applite.d/pstn.conf");
  } else {
    // Nomalize the path
    if (path.empty())
      path = "./";
    if (path[path.size() - 1] != '/')
      path.push_back('/');
  }

  audio_event_handler handler(read_fd, write_fd, uid, key, name, mixed,
      samples, idle, batch, path.c_str(), !batch && reserved != 0);

  return handler.run();
}

