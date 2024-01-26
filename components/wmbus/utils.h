#pragma once

#include "esphome/core/log.h"

// Helper macros, collides with MSVC's stdlib.h unless NOMINMAX is used
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

namespace esphome {
namespace wmbus {

  uint16_t packetSize(uint8_t t_L);
  uint16_t byteSize(uint16_t t_packetSize);

}
}