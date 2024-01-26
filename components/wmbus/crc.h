#pragma once

#include "esphome/core/log.h"

namespace esphome {
namespace wmbus {


  uint16_t crc16(uint8_t const t_message[], uint8_t t_nBytes, uint16_t t_polynomial, uint16_t t_init);
  bool crcValid(const uint8_t *t_bytes, uint8_t t_crcOffset);

}
}