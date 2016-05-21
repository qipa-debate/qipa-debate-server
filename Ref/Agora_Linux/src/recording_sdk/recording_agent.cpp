#include <cerrno>
#include <cstring>
#include <cinttypes>

#include "error_code.h"
#include "base/log.h"
#include "protocol/app_comm.h"
#include "recording_sdk/recording_agent.h"

namespace agora {
namespace recorder {
__attribute__((visibility("default"))) IRecordSdk* IRecordSdk::createInstance(
    IRecordEventHandler *callback) {
  return new RecordStub(callback);
}

__attribute__((visibility("default"))) void IRecordSdk::destroyInstance(
    IRecordSdk *inst) {
  delete inst;
}

RecordStub::RecordStub(IRecordEventHandler *callback)
:started_(false), controller_("agora_recorder") {
  callback_ = callback;
}

RecordStub::~RecordStub() {
}

void RecordStub::joinChannel(const char *vendorKey,
    const char *channelName, uint32_t uid, uint16_t minPort, uint16_t maxPort,
    const char *dir_prefix) {
  if (started_) {
    LOG(ERROR, "%s called twice: %s, %s, %" PRIu32, __func__, vendorKey,
        channelName, uid);
    return;
  }

  channel_name_ = channelName;

  if (!controller_.create_app(vendorKey, channelName, uid, minPort, maxPort,
        true, 8000, dir_prefix)) {
    // Failed to spawn a new applite, notify the PSTN of this event
    callback_->onError(ERROR_APPLITE_FORK_FAILED, "Failed to spawn the applite");
    return;
  }

  started_ = true;
}

void RecordStub::leave() {
  if (!started_) {
    LOG(ERROR, "%s should be called only after joinChannel", __func__);
    return;
  }

  if (!controller_.leave_channel()) {
    // Failed to send leave meassge to the applite
    if (controller_.good()) {
      int err = ERROR_APPLITE_NOT_RESPONDING;
      const char *prompt = "The recorder is busy";
      callback_->onError(err, prompt);
    }
  }
  controller_.close_controller();
  started_ = false;
}

void RecordStub::onTimer() {
  if (!started_) {
    LOG(ERROR, "%s should be called only after joinChannel", __func__);
    return;
  }

  base::unpacker pk;
  uint16_t uri;

  using namespace pstn;

  base::ReadState state;
  while ((state = controller_.read_packet(&pk, &uri)) == base::kReceived) {
    switch (uri) {
    case pstn::JOIN_REPLY_URI: {
      p_join_reply reply;
      reply.unmarshall(pk);
      if (!handle_join_reply(0, 0, reply.code, reply.info))
        return;
      break;
    }
    case pstn::JOIN_REPLY2_URI: {
      p_join_reply2 reply;
      reply.unmarshall(pk);
      if (!handle_join_reply(reply.cid, reply.uid, reply.code, reply.info))
        return;
      break;
    }
    case pstn::VOICE_DATA_URI: {
      p_voice_data data;
      data.unmarshall(pk);
      if (!handle_voice_data(0, data.data))
        return;
      break;
    }
    case pstn::VOICE_DATA2_URI: {
      p_voice_data2 data;
      data.unmarshall(pk);
      if (!handle_voice_data(data.uid, data.data))
        return;
      break;
    }
    case pstn::LEAVE_REPLY_URI: {
      p_leave_reply reply;
      handle_leave_reply();
      break;
    }
    case pstn::ERROR_REPORT_URI: {
      p_applite_error error;
      error.unmarshall(pk);
      if (!handle_recorder_error(error.code, error.info))
        return;
      break;
    }
    case pstn::SESSION_CREATION_URI: {
      p_session_event session;
      session.unmarshall(pk);
      if (!handle_session_creation(session.session_id))
        return;
      break;
    }
    case pstn::SPEAKER_JOIN_URI: {
      p_speaker_join speaker_info;
      speaker_info.unmarshall(pk);
      if (!handle_speaker_info(speaker_info.uid, speaker_info.path))
        return;
      break;
    }
    case APPLITE_IDLE_URI: {
      p_channel_idle idle;
      idle.unmarshall(pk);
      if (!handle_channel_idle(idle.idle))
        return;
      break;
    }
    default: break;
    }
  }

  if (state == base::kEof || state == base::kError) {
    int err = errno;
    LOG(INFO, "Error in reading data: %d", err);
    err = ERROR_APPLITE_NOT_RESPONDING;
    const char *prompt = "The recorder is busy";
    if (!controller_.good()) {
      err = ERROR_APPLITE_CLOSED;
      prompt = "The recorder is vanished. See syslog for more details";
    }
    callback_->onError(err, prompt);
  }
}

bool RecordStub::handle_leave_reply() {
  callback_->onError(-1, "channel leaved");
  // Don't continue to receive packets
  return false;
}

bool RecordStub::handle_voice_data(unsigned int uid, const std::string &data) {
  (void)uid;
  (void)data;
  return true;
}

bool RecordStub::handle_join_reply(uint32_t cid, uint32_t uid, int code,
    const std::string &info) {
  (void)cid;
  (void)code;
  callback_->onJoinSuccess(channel_name_.c_str(), uid, info.c_str());
  return true;
}

bool RecordStub::handle_session_creation(const std::string &sessionId) {
  callback_->onSessionCreate(sessionId.c_str());
  return true;
}

bool RecordStub::handle_recorder_error(int code, const std::string &info) {
  LOG(INFO, "Error in recorder %d", code);
  callback_->onError(code, info.c_str());
  return true;
}

bool RecordStub::handle_speaker_info(uint32_t uid, const std::string &path) {
  LOG(INFO, "Speaker %u joined, path: %s", uid, path.c_str());
  callback_->onSpeakerJoined(uid, path.c_str());
  return true;
}

bool RecordStub::handle_channel_idle(int32_t idle) {
  char buf[80];
  snprintf(buf, 80, "Idle channel detected, over %d seconds", idle);
  callback_->onError(ERROR_APPLITE_CLOSED, buf);
  return true;
}

}
}

