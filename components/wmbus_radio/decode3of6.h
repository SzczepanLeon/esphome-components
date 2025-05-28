#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace esphome {
namespace wmbus_radio {
std::optional<std::vector<uint8_t>>
decode3of6(std::vector<uint8_t> &coded_data);
size_t encoded_size(size_t decoded_size);
} // namespace wmbus_radio
} // namespace esphome