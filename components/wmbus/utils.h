#pragma once

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <algorithm> 
#include <assert.h>
#include <memory.h>
#include <string>
#include <vector>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

namespace esphome {
namespace wmbus {

// ESPHome 2026.11 removed the allocating format_hex_pretty() helper.
// Keep the old W-MBus output (uppercase hex without separators) while
// using the new caller-provided-buffer API.
inline std::string format_wmbus_hex(const std::vector<unsigned char> &data) {
  std::vector<char> buffer(data.size() * 2 + 1, '\0');
  format_hex_pretty_to(buffer.data(), buffer.size(), data.data(), data.size(), '\0');
  return std::string(buffer.data());
}

  uint16_t packetSize(uint8_t t_L);
  uint16_t byteSize(uint16_t t_packetSize);

  unsigned char *safeButUnsafeVectorPtr(std::vector<unsigned char> &v);
  void xorit(unsigned char *srca, unsigned char *srcb, unsigned char *dest, int len);
  void incrementIV(unsigned char *iv, int len);

  bool decrypt_ELL_AES_CTR(std::vector<unsigned char> &frame, std::vector<unsigned char> &aeskey);
  bool decrypt_TPL_AES_CBC_IV(std::vector<unsigned char> &frame, std::vector<unsigned char> &aeskey);

}
}