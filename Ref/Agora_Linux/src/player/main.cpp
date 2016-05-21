// #include <atomic>
#include <cstdint>
#include <cinttypes>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "base/atomic.h"
#include "base/opt_parser.h"
#include "player/engine_driver.h"

using namespace std;
using namespace agora;
using namespace agora::base;
using namespace agora::media;
using namespace agora::player;

int main(int argc, char *argv[]) {
  int32_t min_buf_delay = 100;
  int32_t max_buf_delay = 200;

  int32_t samples = 8000;
  int speed = 1;

  string input_file;
  string output_file;

  cout << "agora player 2.0, compiled on version " << GIT_DESC << endl;

  opt_parser parser;
  parser.add_long_opt("min_buf_delay", &min_buf_delay);
  parser.add_long_opt("max_buf_delay", &max_buf_delay);
  parser.add_long_opt("sample_rates", &samples);
  parser.add_long_opt("input", &input_file);
  parser.add_long_opt("output", &output_file);
  parser.add_long_opt("speed", &speed);

  if (!parser.parse_opts(argc, argv) || input_file.empty()
      || output_file.empty() || (samples != 8000 && samples != 16000)) {
    parser.print_usage(argv[0], cout);
    return -1;
  }

  switch (speed) {
  case 1:
  case 2:
  case 5:
  case 10: break;
  default:
    cerr << "Acceleration ratio should be in 1, 2, 5 or 10" << endl;
    return -1;
  }

  engine_driver driver(samples);

  return !driver.play_file(input_file.c_str(), output_file.c_str(), speed);
}

