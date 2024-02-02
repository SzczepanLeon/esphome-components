#pragma once

#include "esphome/core/log.h"
#include <assert.h>
#include <memory.h>
#include <vector>

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

  unsigned char *safeButUnsafeVectorPtr(std::vector<unsigned char> &v);
  void xorit(unsigned char *srca, unsigned char *srcb, unsigned char *dest, int len);
  void incrementIV(unsigned char *iv, int len);

  bool decrypt_ELL_AES_CTR(std::vector<unsigned char> &frame, std::vector<unsigned char> &aeskey);
  bool decrypt_TPL_AES_CBC_IV(std::vector<unsigned char> &frame, std::vector<unsigned char> &aeskey);

}
}