#include "player/record_file_reader.h"

#include <cstring>
#include <iostream>
#include <memory>

#include "internal/recap_format.h"

using namespace std;

namespace agora {
using namespace recorder;

namespace player {
record_file_reader::record_file_reader() {
  fp_ = NULL;

  is_newer_version_ = false;

  cid_ = 0;
  uid_ = 0;
  start_ms_ = 0;
}

record_file_reader::~record_file_reader() {
  if (fp_) {
    fclose(fp_);
  }
}

bool record_file_reader::open(const char *filename) {
  fp_ = fopen(filename, "rb");
  if (!fp_) {
    cerr << "Failed to open the record file: " << filename << ", Reason: "
        << strerror(errno) << endl;
    return false;
  }

  static const size_t kCommon = sizeof(recorder_common_header);
  recording_file_header header;
  if (fread(&header, kCommon, 1, fp_) != 1) {
    cerr << "Corrupted file: " << filename << endl;
    return false;
  }

  if (header.common.magic_id != AGORA_MAGIC_ID) {
    cerr << "Not an agora file: " << filename << endl;
    return false;
  }

  if (header.common.version.major_version == 1) {
    char *residual = reinterpret_cast<char *>(&header) + kCommon;
    if (fread(residual, sizeof(header.header1) - kCommon, 1, fp_) != 1) {
      cerr << "Corrupted recording file: " << filename << endl;
      return false;
    }

    cid_ = header.header1.cid;
    uid_ = header.header1.uid;
    start_ms_ = header.header1.start_ms;

    cout << "Replaying the file: " << filename << endl;
    cout << "Channel: " << cid_ << endl;
    cout << "Uid: " << uid_ << endl;

    time_t start = start_ms_ / 1000;
    cout << "UTC Timestamp in ms: " << start_ms_ << ", "
      << ctime(&start) << endl;
  } else if (header.common.version.major_version == 2) {
    char *residual = reinterpret_cast<char *>(&header) + kCommon;
    if (fread(residual, sizeof(header.header2) - kCommon, 1, fp_) != 1) {
      cerr << "Corrupted recording file, version 2: " << filename << endl;
      return false;
    }

    is_newer_version_ = true;
    cname_ = header.header2.channel_name;
    uid_ = header.header2.uid;
    start_ms_ = header.header2.start_ms;
    // FIXME(liuyong):
    // read audio engine parameters from files
    fseek(fp_, header.header2.data_offset, SEEK_SET);
  } else {
    cerr << "This player only supports files of version 1-2\n";
    return false;
  }

  return true;
}

bool record_file_reader::get_next_packet(audio_packet *ppacket_header, void **buf) {
  if (ppacket_header == NULL || buf == NULL) {
    cerr << "Invalid packet address " << ppacket_header << endl;
    return false;
  }

  if (fread(ppacket_header, sizeof(*ppacket_header), 1, fp_) != 1) {
    if (feof(fp_)) {
      cerr << "Reached the end of the file " << endl;
      return false;
    }

    cerr << "Error in reading packets: " << strerror(errno) << endl;
    return false;
  }

  const audio_packet &p = *ppacket_header;
  std::unique_ptr<char[]> payload(new char[p.payload_size]);

  if (fread(payload.get(), p.payload_size, 1, fp_) != 1) {
    cerr << "Incomplete packet at pos: " << ftell(fp_) << endl;
    return false;
  }

  // cout << "payload type: " << p.payload_type << ", seq number: "
  //     << p.seq_number << ", timestamp: " << p.timestamp << endl;

  *buf = payload.get();
  payload.release();

  return true;
}

}
}

