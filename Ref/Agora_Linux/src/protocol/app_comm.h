#pragma once
#include <string>

#include "base/packer.h"
#include "base/packet.h"

namespace agora {
namespace pstn {
using base::packet;
using base::packer;
using base::unpacker;

enum {EMPTY_URI=0, JOIN_REPLY_URI, VOICE_DATA_URI, LEAVE_REQUEST_URI,
  LEAVE_REPLY_URI, ERROR_REPORT_URI, PHONE_EVENT_CHANGE_URI, VOICE_DATA2_URI,
  SESSION_CREATION_URI, JOIN_REPLY2_URI, SPEAKER_JOIN_URI, APPLITE_IDLE_URI,
  JOIN_REPLY3_URI, USER_JOINED_URI, USER_OFFLINE_URI,
};

DECLARE_PACKET_2(p_join_reply, JOIN_REPLY_URI, int, code, std::string, info);
DECLARE_PACKET_4(p_join_reply2, JOIN_REPLY2_URI, int, cid, int, uid, \
    int, code, std::string, info);
DECLARE_PACKET_4(p_join_reply3, JOIN_REPLY3_URI, std::string, channel, \
    uint32_t, uid, int, code, std::string, info);

DECLARE_PACKET_2(p_applite_error, ERROR_REPORT_URI, int, code, std::string, info);
DECLARE_PACKET_1(p_voice_data, VOICE_DATA_URI, std::string, data);
DECLARE_PACKET_2(p_voice_data2, VOICE_DATA2_URI, unsigned int, uid, std::string, data);
DECLARE_PACKET_1(p_leave_request, LEAVE_REQUEST_URI, int, from);
DECLARE_PACKET_1(p_leave_reply, LEAVE_REPLY_URI, int, code);
DECLARE_PACKET_1(p_phone_event, PHONE_EVENT_CHANGE_URI, int, status);
DECLARE_PACKET_1(p_session_event, SESSION_CREATION_URI, std::string, session_id);
DECLARE_PACKET_2(p_speaker_join, SPEAKER_JOIN_URI, uint32_t, uid, std::string, path);
DECLARE_PACKET_1(p_channel_idle, APPLITE_IDLE_URI, uint32_t, idle);
DECLARE_PACKET_2(p_user_joined, USER_JOINED_URI, uint32_t, uid, int, elapsed);
DECLARE_PACKET_2(p_user_offline, USER_OFFLINE_URI, uint32_t, uid, int, reason);
}
}

