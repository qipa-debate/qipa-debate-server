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
#include "applite/audio_event_handler.h"

#include "base/atomic.h"

using namespace std;
using namespace agora;
using namespace agora::base;
using namespace agora::pstn;

bool g_record_pcm = false;
int g_recorded_time_span = 3600; // for an hour

std::string g_record_directory;

void print_version() {
  std::cout << "Applite, version " << GIT_DESC << std::endl;
}

void print_usage() {
  const char *usage =
    "applite --help\n"
    "Display the help\n\n"
    "applite --version\n"
    "Display the version info\n\n"
    "applite --batch --key YOUR_VENDOR_KEY --name CHANNEL_NAME [--uid UINT32]\n"
    "    [--audio_mix 0|1] [--samples 8000|16000] [--idle IDLE_SECONDS]\n"
    "    [--path STORE_DIR_PREFIX] [--slice SLICE_MINUTES]\n"
    "OPTIONS:\n"
    "  --batch      Run the applite as a standalone application, (required)\n"
    "  --key        Specify the vendor key for joining a channel\n"
    "  --name       Specify the name of the channel to be joined\n"
    "  --uid        Specify the uid as which this applite is joining\n"
    "  --audio_mix  Specify the audio mode: 0, non-mixed; 1, mixed(default)\n"
    "  --samples    Specify the sampling rate of audio: 8K(default) or 16K\n"
    "  --idle       Specify the interval in seconds before the applite "
    "decides\n"
    "               to quit an inactive channel. For example, given a value\n"
    "               of 30, the applite will quit a channel after it contains\n"
    "               nobody over 30 seconds. The minimum accepted is 20. \n"
    "               Default is no idle checking.\n"
    "  --path       Specify the path where audio files will be stored(\n"
    "               current directory, default).\n"
    "  --codec      Specify the encoding codec: PCM(default) or mp3\n"
    "  --slice      Specify the interval in minutes in which the applite will\n"
    "               split the audio files apart. Default is no slicing.\n"
    "               If slicing is enabled, the audio files are named as\n"
    "               {CHANNEL_NAME}_uid_{slice_number}.pcm/mp3";
  cerr << usage << endl;
}

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
  int32_t slice = 10000000;
  int read_fd = -1;
  int write_fd = -1;
  uint32_t uid = 0;
  int idle = 120000000;
  string key;
  string name;
  string path;
  string codec("pcm");

  bool batch = false;
  bool get_version = false;
  bool get_help = false;
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
  parser.add_long_opt("version", &get_version);
  parser.add_long_opt("help", &get_help);
  parser.add_long_opt("slice", &slice);
  parser.add_long_opt("codec", &codec);

  if (!parser.parse_opts(argc, argv)) {
    cerr << "Illegal arguments\n";
    print_usage();
    return -1;
  }

  if (get_version) {
    print_version();
    return 0;
  }

  if (get_help) {
    print_usage();
    return 0;
  }

  if (samples != 8000 && samples != 16000) {
    print_usage();
    // parser.print_usage(argv[0], cout);
    return -2;
  }

  if (key.empty() || name.empty()) {
    cerr << "Vendor key or channel name should not be empty\n";
    return -1;
  }

  for (string::iterator it = codec.begin(); it != codec.end(); ++it) {
    *it = std::tolower(*it);
  }

  if (codec != "pcm" && codec != "mp3") {
    cerr << "Unsupported codec: " << codec << ", only PCM and MP3 supported";
    print_usage();
    return -3;
  }

  idle = (std::max)(idle, 20);

  LOG(INFO, "uid %" PRIu32 " from vendor %s is joining channel %s, mode: %s",
      uid, key.c_str(), name.c_str(), batch ? "batch" : "programmed");

  if (!batch && (read_fd <= 0 || write_fd <= 0)) {
    print_usage();
    LOG(ERROR, "Illegal arguments: batch: %d, read_fd %d, write_fd %d",
        batch, read_fd, write_fd);
    return -3;
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
      samples, idle, batch, path.c_str(), !batch && reserved != 0,
      slice * 60, codec.c_str());

  return handler.run();
}

