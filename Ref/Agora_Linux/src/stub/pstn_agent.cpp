#include <cerrno>
#include <cstring>
#include <cinttypes>

#include "include/error_code.h"
#include "base/log.h"
#include "protocol/app_comm.h"
#include "stub/pstn_agent.h"

namespace agora {
namespace pstn {
__attribute__((visibility("default"))) IAgoraSdk* IAgoraSdk::createInstance(
    IAgoraSdkEventHandler *callback) {
  return new PstnStub(callback);
}

__attribute__((visibility("default"))) void IAgoraSdk::destroyInstance(
    IAgoraSdk *inst) {
  delete inst;
}

PstnStub::PstnStub(IAgoraSdkEventHandler *callback) :started_(false) {
  callback_ = callback;
}

PstnStub::~PstnStub() {
}

void PstnStub::joinChannel(const char *vendorID, const char *vendorKey,
    const char *channelName, uint32_t uid, uint16_t minPort, uint16_t maxPort,
    bool mixed, int32_t sampleRates, uint32_t reserved, int linger_time) {
  if (started_) {
    LOG(ERROR, "%s called twice: %s, %s, %" PRIu32, __func__, vendorKey,
        channelName, uid);
    return;
  }

  (void)vendorID;
  if (sampleRates != 8000 && sampleRates != 16000) {
    callback_->onError(ERROR_APPLITE_FORK_FAILED, "Invalid sample rates: only "
        "8000 or 16000 allowed");
    return;
  }

  channel_name_ = channelName;

  if (!controller_.create_app(vendorKey, channelName, uid, minPort, maxPort,
        mixed, sampleRates, NULL, reserved, linger_time)) {
    // Failed to spawn a new applite, notify the PSTN of this event
    callback_->onError(ERROR_APPLITE_FORK_FAILED, "Failed to spawn the applite");
    return;
  }

  started_ = true;
}

void PstnStub::leave() {
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

void PstnStub::sendVoiceData(const char *pbuffer, uint32_t length) {
  if (!started_) {
    LOG(ERROR, "%s should be called only after joinChannel", __func__);
    return;
  }

  LOG_EVERY_N(DEBUG, 100, "PSTN SendVoiceData: %" PRIu32, length);

  if (!controller_.send_voice_data(pbuffer, length)) {
    int err = ERROR_APPLITE_NOT_RESPONDING;
    const char *prompt = "The recorder is busy";
    if (!controller_.good()) {
      err = ERROR_APPLITE_CLOSED;
      prompt = "The recorder is vanished. See syslog for more details";
    }
    callback_->onError(err, prompt);
  }
}

void PstnStub::onTimer() {
  if (!started_) {
    LOG(ERROR, "%s should be called only after joinChannel", __func__);
    return;
  }

  base::unpacker pk;
  uint16_t uri;

  base::ReadState state;
  while ((state = controller_.read_packet(&pk, &uri)) == base::kReceived) {
    switch (uri) {
    case JOIN_REPLY_URI: {
      p_join_reply reply;
      reply.unmarshall(pk);
      if (!handle_join_reply(0, 0, reply.code, reply.info))
        return;
      break;
    }
    case JOIN_REPLY2_URI: {
      p_join_reply2 reply;
      reply.unmarshall(pk);
      if (!handle_join_reply(reply.cid, reply.uid, reply.code, reply.info))
        return;
      break;
    }
    case JOIN_REPLY3_URI: {
      p_join_reply3 reply;
      reply.unmarshall(pk);
      if (!handle_join_reply(0, reply.uid, reply.code, reply.info))
        return;
      break;
    }
    case VOICE_DATA_URI: {
      p_voice_data data;
      data.unmarshall(pk);
      if (!handle_voice_data(0, data.data))
        return;
      break;
    }
    case VOICE_DATA2_URI: {
      p_voice_data2 data;
      data.unmarshall(pk);
      if (!handle_voice_data(data.uid, data.data))
        return;
      break;
    }
    case LEAVE_REPLY_URI: {
      p_leave_reply reply;
      handle_leave_reply();
      break;
    }
    case ERROR_REPORT_URI: {
      p_applite_error error;
      error.unmarshall(pk);
      if (!handle_applite_error(error.code, error.info))
        return;
      break;
    }
    case SESSION_CREATION_URI: {
      p_session_event session;
      session.unmarshall(pk);
      if (!handle_session_creation(session.session_id))
        return;
      break;
    }
    case SPEAKER_JOIN_URI: {
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
    case USER_JOINED_URI: {
      p_user_joined joined;
      joined.unmarshall(pk);
      if (!handle_user_joined(joined.uid, joined.elapsed))
        return;
      break;
    }
    case USER_OFFLINE_URI: {
      p_user_offline offline;
      offline.unmarshall(pk);
      if (!handle_user_offline(offline.uid, offline.reason))
        return;
      break;
    }
    default: break;
    }
  }

  if (state == base::kEof || state == base::kError) {
    int err = errno;
    LOG(INFO, "Error in reading data: %d, %d", err, state);
    err = ERROR_APPLITE_NOT_RESPONDING;
    const char *prompt = "The recorder is busy";
    if (!controller_.good()) {
      err = ERROR_APPLITE_CLOSED;
      prompt = "The recorder is vanished. See syslog for more details";
    }
    callback_->onError(err, prompt);
  }
}

bool PstnStub::handle_leave_reply() {
  callback_->onError(-1, "channel leaved");
  // Don't continue to receive packets
  return false;
}

bool PstnStub::handle_voice_data(unsigned int uid, const std::string &data) {
  callback_->onVoiceData(uid, &data[0], static_cast<uint32_t>(data.size()));
  return true;
}

bool PstnStub::handle_join_reply(uint32_t cid, uint32_t uid, int code,
    const std::string &info) {
  (void)cid;
  (void)code;
  callback_->onJoinSuccess(channel_name_.c_str(), uid, info.c_str()); 
  return true;
}

bool PstnStub::handle_session_creation(const std::string &sessionId) {
  callback_->onSessionCreate(sessionId.c_str());
  return true;
}

bool PstnStub::handle_applite_error(int code, const std::string &info) {
  LOG(INFO, "Error in applite %d", code);
  callback_->onError(code, info.c_str());
  return true;
}

bool PstnStub::handle_speaker_info(uint32_t uid, const std::string &path) {
  LOG(INFO, "Speaker %u joined, path: %s", uid, path.c_str());
  return true;
}

bool PstnStub::handle_channel_idle(int32_t idle) {
  char buf[80];
  snprintf(buf, 80, "Idle channel detected, over %d seconds", idle);
  callback_->onError(ERROR_APPLITE_CLOSED, buf);
  return true;
}

bool PstnStub::handle_user_joined(uint32_t uid, int elapsed) {
  (void)elapsed;
  callback_->onUserJoined(uid);
  return true;
}

bool PstnStub::handle_user_offline(uint32_t uid, int reason) {
  callback_->onUserOffline(uid, static_cast<IAgoraSdkEventHandler::
      OfflineReason>(reason));

  return true;
}

void PstnStub::notifyPhoneEvent(PhoneEvent status) {
  if (!controller_.notify_phone_event(static_cast<int>(status))) {
    // Failed to send phone event to the applite
    int err = ERROR_APPLITE_NOT_RESPONDING;
    const char *prompt = "The recorder is busy";
    if (!controller_.good()) {
      err = ERROR_APPLITE_CLOSED;
      prompt = "The recorder is vanished. See syslog for more details";
    }
    callback_->onError(err, prompt);
  }
}

}
}

