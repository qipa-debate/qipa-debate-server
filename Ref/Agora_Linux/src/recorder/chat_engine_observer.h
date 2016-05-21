#pragma once

#include <cstdint>
#include <string>

namespace agora {
namespace recorder {
struct chat_engine_observer {
  virtual void onSpeakerJoined(uint32_t uid, const std::string &path) = 0;
};

}
}

