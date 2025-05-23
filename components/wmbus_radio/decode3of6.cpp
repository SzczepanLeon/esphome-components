#include "decode3of6.h"

#include <map>

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome
{
  namespace wmbus_radio
  {
    static const char *TAG = "3of6";
    std::optional<std::vector<uint8_t>> decode3of6(std::vector<uint8_t> &coded_data)
    {

      static const std::map<uint8_t, uint8_t> lookupTable = {
          {0b010110, 0x0},
          {0b001101, 0x1},
          {0b001110, 0x2},
          {0b001011, 0x3},
          {0b011100, 0x4},
          {0b011001, 0x5},
          {0b011010, 0x6},
          {0b010011, 0x7},
          {0b101100, 0x8},
          {0b100101, 0x9},
          {0b100110, 0xA},
          {0b100011, 0xB},
          {0b110100, 0xC},
          {0b110001, 0xD},
          {0b110010, 0xE},
          {0b101001, 0xF},
      };

      // ESP_LOGD(TAG, "Decoding 3of6 data: %s", format_hex(coded_data).c_str());

      std::vector<uint8_t> decodedBytes;
      auto segments = coded_data.size() * 8 / 6;
      auto data = coded_data.data();

      for (size_t i = 0; i < segments; i++)
      {
        auto bit_idx = i * 6;
        auto byte_idx = bit_idx / 8;
        auto bit_offset = bit_idx % 8;

        uint8_t code = (data[byte_idx] << bit_offset);
        if (bit_offset > 0)
          code |= (data[byte_idx + 1] >> (8 - bit_offset));
        code >>= 2;

        auto it = lookupTable.find(code);
        if (it == lookupTable.end())
        {
          // ESP_LOGW(TAG, "Invalid code: 0x%02X", code);
          return {};
        }

        if (i % 2 == 0)
          decodedBytes.push_back(it->second << 4);
        else
          decodedBytes.back() |= it->second;
      }

      // ESP_LOGV(TAG, "Successfully decoded %zu bytes", decodedBytes.size());
      return decodedBytes;
    }

    size_t encoded_size(size_t decoded_size)
    {
      // Every 2 bytes (4 nibbles by 6 bits = 24b) of decoded data is encoded into 3 bytes of coded data
      // +1 for rounding up
      return (3 * decoded_size + 1) / 2;
    }
  }
}