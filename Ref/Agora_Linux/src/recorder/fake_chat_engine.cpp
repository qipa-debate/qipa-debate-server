#include "recorder/fake_chat_engine.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "base/safe_log.h"
#include "internal/recap_format.h"
#include "recorder/active_stream.h"
#include "recorder/audio_event_handler.h"

using namespace std;

extern std::string g_channel_name;

namespace agora {
using namespace media;

namespace recorder {
inline parameter_value::parameter_value(bool value) {
  type_ = kBool;
  b_ = value;
}

inline parameter_value::parameter_value(int value) {
  type_ = kInt;
  i_ = value;
}

inline parameter_value::parameter_value(double value) {
  type_ = kDouble;
  d_ = value;
}

inline parameter_value::parameter_value(const char *value) {
  type_ = kString;
  str_  = string(value);
}

inline parameter_value::parameter_value(const string &value) {
  type_ = kString;
  str_ = value;
}

inline parameter_value::value_type parameter_value::get_type() const {
  return type_;
}

inline bool parameter_value::get_as_bool() const {
  assert(type_ == kBool);
  return b_;
}

inline int parameter_value::get_as_int() const {
  assert(type_ == kInt);
  return i_;
}

inline double parameter_value::get_as_double() const {
  assert(type_ == kDouble);
  return d_;
}

inline const string& parameter_value::get_as_string() const {
  assert(type_ == kString);
  return str_;
}

fake_chat_engine::fake_chat_engine(chat_engine_observer *observer,
    const string &prefix_dir) {
  observer_ = observer;

  is_calling_ = false;
  record_packet_ = true;

  prefix_dir_ = prefix_dir;
  channel_name_ = g_channel_name;
}

fake_chat_engine::~fake_chat_engine() {
  clear_streams();
}

void fake_chat_engine::release() {
  SAFE_LOG(INFO) << "Fake chat engine is ready to release";
}

int fake_chat_engine::init() {
  return 0;
}

int fake_chat_engine::init(IChatEngineObserver *observer) {
  (void)observer;
  return 0;
}

int fake_chat_engine::initVideo() {
  SAFE_LOG(ERROR) << "No video engine supported";
  return -1;
}

class real_string : public media::util::IString {
 public:
  explicit real_string(const std::string &str);
  virtual ~real_string();

  virtual bool empty() const;
  virtual const char* c_str();
  virtual const char* data();
  virtual size_t length();
  virtual void release();
 private:
  std::string data_;
};

real_string::real_string(const std::string &str) :data_(str) {
}

real_string::~real_string() {
}

bool real_string::empty() const {
  return data_.empty();
}

const char* real_string::c_str() {
  return data_.c_str();
}

const char* real_string::data() {
  return data_.data();
}

size_t real_string::length() {
  return data_.length();
}

void real_string::release() {
  delete this;
}

int fake_chat_engine::getParameter(const char *key, const char *args,
    media::util::AString &result) {
  (void)args;

  auto it = parameters_.find(string(key));
  if (it == parameters_.end()) {
    SAFE_LOG(WARN) << "No parameter " << key << " found";
    return -1;
  }

  if (it->second.get_type() != parameter_value::kString) {
    SAFE_LOG(WARN) << "parameter " << key << " is not a string type";
    return -1;
  }

  result.reset(new (std::nothrow)real_string(it->second.get_as_string()));
  return 0;
}

int fake_chat_engine::setBoolParameter(const char *param, bool value) {
  SAFE_LOG(INFO) << "Param: " << param << ": " << value;
  parameters_[string(param)] = parameter_value(value);
  return 0;
}

int fake_chat_engine::setIntParameter(const char *param, int value) {
  SAFE_LOG(INFO) << "Param: " << param << ": " << value;
  parameters_[string(param)] = parameter_value(value);
  return 0;
}

int fake_chat_engine::setNumberParameter(const char *param, double value) {
  SAFE_LOG(INFO) << "Param: " << param << ": " << value;
  parameters_[string(param)] = parameter_value(value);
  return 0;
}

int fake_chat_engine::setStringParameter(const char *param, const char *value) {
  SAFE_LOG(INFO) << "Param: " << param << ": " << value;
  parameters_[string(param)] = parameter_value(value);
  return 0;
}

int fake_chat_engine::setObjectParameter(const char *param, const char *val) {
  SAFE_LOG(INFO) << "Param: " << param << ": " << val;
  parameters_[string(param)] = parameter_value(val);
  return 0;
}

int fake_chat_engine::terminate() {
  is_calling_ = false;
  return 0;
}

int fake_chat_engine::startCall() {
  SAFE_LOG(INFO) << "startCall";
  is_calling_ = true;
  return 0;
}

int fake_chat_engine::stopCall() {
  is_calling_ = false;
  std::lock_guard<std::mutex> lock(stream_lock_);
  streams_.clear();
  return 0;
}

bool fake_chat_engine::isCalling() {
  return is_calling_;
}

int fake_chat_engine::setParameters(const char *parameters) {
  (void)parameters;
  // assert(false);
  SAFE_LOG(WARN) << "setParameters " << parameters;
  return -1;
}

int fake_chat_engine::getParameters(const char *parameters, char *buffer,
    size_t *length) {
  (void)parameters;
  (void)buffer;
  (void)length;
  SAFE_LOG(WARN) << "getParameters " << parameters;
  return -1;
}

media::IAudioEngine* fake_chat_engine::getAudioEngine() {
  return this;
}

media::IVideoEngine* fake_chat_engine::getVideoEngine() {
  SAFE_LOG(ERROR) << "getVideoEngine called, not implemented yet";
  return NULL;
}

int fake_chat_engine::getEngineEvents(media::ChatEngineEventData &events) {
  events = media::ChatEngineEventData();
  return 0;
}

int fake_chat_engine::registerTransport(IAudioTransport *transport) {
  SAFE_LOG(INFO) << "Registering the audio transport";

  transport_ = transport;
  return 0;
}

int fake_chat_engine::deRegisterTransport() {
  SAFE_LOG(INFO) << "Unregistering the audio transport";

  transport_ = NULL;
  return 0;
}

int fake_chat_engine::receiveNetPacket(unsigned int uid,
    const unsigned char *payload,
    unsigned short payload_size,
    int payload_type,
    unsigned int timestamp,
    unsigned short seq_number) {
  // FIXME(liuyong): The most important function.
  if (!is_calling_) {
    SAFE_LOG(ERROR) << __func__ << " should be called after startCall";
    return -1;
  }

  std::lock_guard<std::mutex> lock(stream_lock_);
  if (streams_.find(uid) == streams_.end()) {
    SAFE_LOG(INFO) << "New user " << uid << " joined " << channel_name_;
    streams_[uid] = new active_stream(prefix_dir_, channel_name_, uid);

    if (!streams_[uid]->open()) {
      return -1;
    }

    observer_->onSpeakerJoined(uid, streams_[uid]->get_recorder_file());
  }

  streams_[uid]->record_net_packet(payload, payload_size, payload_type,
      timestamp, seq_number);
  return 0;
}

int fake_chat_engine::registerNetEQStatistics(IJitterStatistics *neteq) {
  SAFE_LOG(INFO) << "NetEQ callback registered, but I cannot support "
      "this feature";

  (void)neteq;
  return 0;
}

int fake_chat_engine::deRegisterNetEQStatistics() {
  SAFE_LOG(INFO) << "NetEQ callback unregistered";
  return 0;
}

int fake_chat_engine::registerChatEngineObserver(IChatEngineObserver *observer) {
  SAFE_LOG(INFO) << "ChatEngine observer registered";

  (void)observer;
  return 0;
}

int fake_chat_engine::deRegisterChatEngineObserver() {
  SAFE_LOG(INFO) << "ChatEngine observer unregistered";
  return 0;
}

bool fake_chat_engine::isCallInterrupted() {
  return false;
}

int fake_chat_engine::startFileAsInput(const char *filename) {
  SAFE_LOG(ERROR) << __func__ << " called for " << filename;

  return -1;
}

int fake_chat_engine::numOfCodecs() {
  return 1;
}

int fake_chat_engine::getCodecInfo(int index, char *pInfoBuffer, int length) {
  (void)index;
  (void)pInfoBuffer;
  (void)length;

  SAFE_LOG(ERROR) << __func__  << " unsupported";
  return -1;
}

int fake_chat_engine::setCodec(int index) {
  SAFE_LOG(ERROR) << __func__ << " set to " << index;
  return -1;
}

int fake_chat_engine::setMultiFrameInterleave(int num_frame, int interleave) {
  SAFE_LOG(ERROR) << __func__ << " called: " << num_frame
    << " frames per packet, and " << interleave << " frames interleaves";

  return -1;
}

int fake_chat_engine::setREDStatus(bool enable) {
  SAFE_LOG(ERROR) << __func__ << ": " << enable;
  return -1;
}

int fake_chat_engine::setDTXStatus(DTX_MODE dtx_mode) {
  SAFE_LOG(ERROR) << __func__ << " called";

  (void)dtx_mode;
  return -1;
}

int fake_chat_engine::setAecType(AEC_TYPE type) {
  (void)type;

  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::setAudioOutputRouting(AUDIO_OUTPUT_ROUTING routing) {
  (void)routing;

  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::setAgcStatus(bool enable) {
  (void)enable;

  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::setNsStatus(bool enable) {
  (void)enable;

  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::getJitterBufferMaxMetric() {
  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::startDebugRecording(const char *filename) {
  SAFE_LOG(WARN) << __func__ << " unsupported";

  (void)filename;
  return -1;
}

int fake_chat_engine::startDecodeRecording(const char *filename) {
  SAFE_LOG(ERROR) << __func__ << "called for " << filename << ", unsupported";
  return -1;
}

int fake_chat_engine::stopDebugRecording() {
  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::startRecapPlaying() {
  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::setRecapStatus(int reportIntervalMs) {
  SAFE_LOG(ERROR) << __func__ << " unsupported: " << reportIntervalMs;
  return -1;
}

int fake_chat_engine::setNetEQMaximumPlayoutDelay(int delayMs) {
  SAFE_LOG(INFO) << __func__ << ": " << delayMs;
  return 0;
}

int fake_chat_engine::setNetEQMinimumPlayoutDelay(int delayMs) {
  SAFE_LOG(INFO) << __func__ << ": " << delayMs;
  return 0;
}

int fake_chat_engine::setSpeakerStatus(bool enable) {
  SAFE_LOG(INFO) << __func__ << ": " << enable;
  return -1;
}

int fake_chat_engine::getPlayoutTS(unsigned uid, int *playoutTS) {
  (void)uid;
  *playoutTS = 0;
  // SAFE_LOG(INFO) << __func__ << ": " << uid << ", unsupported";
  return 0;
}

int fake_chat_engine::setSpeakersReportStatus(int interval, int factor) {
  SAFE_LOG(ERROR) << __func__ << ": interval " << interval
      << ", smoothFactor " << factor;

  return -1;
}

int fake_chat_engine::startCallRecording(const char *filename) {
  SAFE_LOG(ERROR) << __func__ << " unsupported: " << filename;
  return -1;
}

int fake_chat_engine::stopCallRecording() {
  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::stopDecodeRecording() {
  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::setMuteStatus(bool enable) {
  (void)enable;
  return 0;
}

int fake_chat_engine::setAudioDeviceLayer(AUDIO_LAYERS audioLayer) {
  (void)audioLayer;
  SAFE_LOG(ERROR) << __func__ << " unsupported";
  return -1;
}

int fake_chat_engine::getNumOfRecordingDevices(int &devices) {
  devices = 1;
  return 0;
}

int fake_chat_engine::getNumOfPlayoutDevices(int &devices) {
  devices = 1;
  return 0;
}

int fake_chat_engine::getRecordingDeviceName(int index, char name[],
    char deviceId[]) {
  (void)index;
  strcpy(name, "Agora Fake Device");
  strcpy(deviceId, "0xFFFFFFFF");

  return 0;
}

int fake_chat_engine::getPlayoutDeviceName(int index, char name[],
    char deviceId[]) {
  (void)index;
  strcpy(name, "Agora Fake Playout");
  strcpy(deviceId, "0xEEEEEEEE");

  return 0;
}

int fake_chat_engine::getRecordingDeviceStatus(bool &isAvailable) {
  isAvailable = true;
  return 0;
}

int fake_chat_engine::getPlayoutDeviceStatus(bool &isAvailable) {
  isAvailable = true;
  return 0;
}

int fake_chat_engine::setRecordingDevice(int index) {
  (void)index;
  return 0;
}

int fake_chat_engine::setRecordingDevice(const char deviceId[]) {
  (void)deviceId;
  return 0;
}

int fake_chat_engine::setPlayoutDevice(int index) {
  (void)index;
  return 0;
}

int fake_chat_engine::setPlayoutDevice(const char deviceId[]) {
  (void)deviceId;
  return 0;
}

int fake_chat_engine::getCurrentRecordingDevice(char deviceId[]) {
  strcpy(deviceId, "0xFFFFFFFF");
  return 0;
}

int fake_chat_engine::getCurrentPlayoutDevice(char deviceId[]) {
  strcpy(deviceId, "0xEEEEEEEE");
  return 0;
}

bool fake_chat_engine::load_config() {
  const char *file = "/etc/agora.d/recording.conf";

  bool record = false;
  int timespan = 3600;

  (void)record;

  ifstream fin(file);
  string line;
  while (getline(fin, line)) {
    string opt;
    string arg;
    istringstream sin(line);
    sin >> opt >> arg;
    if (opt.empty() || arg.empty())
      continue;
    if (opt == "record") {
      if (arg == "on")
        record_packet_ = true;
    } else if (opt == "timespan") {
      timespan = atoi(opt.c_str());
      if (timespan < 600)
        timespan = 600;
    } else if (opt == "directory") {
      prefix_dir_ = arg;
    }
  }

  return true;
}

void fake_chat_engine::clear_streams() {
  typedef std::unordered_map<uint32_t, active_stream *> stream_map_t;
  for (stream_map_t::iterator it = streams_.begin(); it != streams_.end();
      ++it) {
    delete it->second;
  }

  streams_.clear();
}

class TraceLogger :public media::ITraceService {
 public:
  TraceLogger();
  virtual ~TraceLogger();

  virtual void release();
  virtual bool startTrace(const char *file, int max_size, unsigned filter);
  virtual bool startTrace(media::ITraceCallback *callback, unsigned filter);
  virtual void stopTrace();
  virtual void setTraceFile(const char *file, int max_size);
  virtual void setTraceCallback(media::ITraceCallback *callback);
  virtual void setTraceFilter(unsigned filter);
  virtual void addTrace(int level, int module, int id, const char *msg);
  virtual void flushTrace();
 private:
  unsigned int filter_;
  media::ITraceCallback *callback_;
};

TraceLogger::TraceLogger() {
  filter_ = 0;
  callback_ = NULL;
}

TraceLogger::~TraceLogger() {
}

void TraceLogger::release() {
  SAFE_LOG(INFO) << "TraceLogger released";
}

bool TraceLogger::startTrace(const char *file, int max_size, unsigned filter) {
  // simply ignore it.
  SAFE_LOG(INFO) << "startTrace: " << file << ", maxFileSize: " << max_size
      << ", filter: " << filter;
  filter_ = filter;
  return true;
}

bool TraceLogger::startTrace(media::ITraceCallback *callback, unsigned filter) {
  SAFE_LOG(INFO) << "startTrace to callback ";
  callback_ = callback;
  filter_ = filter;
  return true;
}

void TraceLogger::stopTrace() {
  SAFE_LOG(INFO) << __func__;
}

void TraceLogger::setTraceFile(const char *file, int max_size) {
  SAFE_LOG(INFO) << "setTraceFile " << file << ", " << max_size;
}

void TraceLogger::setTraceCallback(media::ITraceCallback *callback) {
  callback_ = callback;
}

void TraceLogger::setTraceFilter(unsigned filter) {
  filter_ = filter;
}

void TraceLogger::addTrace(int level, int module, int id, const char *msg) {
  SAFE_LOG(INFO) << " level: " << level << ", module " << module << ", id: "
      << id << ":" << msg;
}

void TraceLogger::flushTrace() {
}

}
}

extern agora::recorder::audio_event_handler *g_audio_handler;
extern std::string g_recording_dir;

extern "C" __attribute__((visibility ("default"))) agora::media::IChatEngine*
createChatEngine(const char *profile, void *context) {
  (void)profile;
  (void)context;

  return new agora::recorder::fake_chat_engine(g_audio_handler, g_recording_dir);
}

extern "C" __attribute__ ((visibility ("default"))) void
destroyChatEngine(agora::media::IChatEngine *engine) {
  typedef agora::recorder::fake_chat_engine fake_engine_t;
  fake_engine_t *e = dynamic_cast<fake_engine_t *>(engine);
  delete e;
}

extern "C" __attribute__((visibility ("default"))) agora::media::ITraceService*
createTraceService() {
  return new agora::recorder::TraceLogger();
}

extern "C" __attribute__ ((visibility ("default"))) const char*
findChatEngineProfile(const char *deviceId) {
  (void)deviceId;
  return NULL;
}

