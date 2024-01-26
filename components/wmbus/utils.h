#pragma once

#include "esphome/core/log.h"

namespace esphome {
namespace wmbus {

  uint16_t packetSize(uint8_t t_L);
  uint16_t byteSize(uint16_t t_packetSize);

}
}