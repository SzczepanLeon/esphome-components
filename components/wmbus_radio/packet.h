#pragma once
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "esphome/components/wmbus_common/wmbus.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace wmbus_radio {
struct Frame;

struct Packet {
  friend class Frame;

public:
  Packet();

  uint8_t *rx_data_ptr();
  size_t rx_capacity();
  bool calculate_payload_size();
  void set_rssi(int8_t rssi);

  std::optional<Frame> convert_to_frame();

protected:
  std::vector<uint8_t> data_;

  size_t expected_size();
  size_t expected_size_ = 0;

  uint8_t l_field();
  int8_t rssi_ = 0;

  LinkMode link_mode();
  LinkMode link_mode_ = LinkMode::UNKNOWN;
};

struct Frame {
public:
  Frame(Packet *packet);

  std::vector<uint8_t> &data();
  LinkMode link_mode();
  int8_t rssi();

  std::vector<uint8_t> as_raw();
  std::string as_hex();
  std::string as_rtlwmbus();

  void mark_as_handled();
  uint8_t handlers_count();

protected:
  std::vector<uint8_t> data_;
  LinkMode link_mode_;
  int8_t rssi_;
  uint8_t handlers_count_ = 0;
};

} // namespace wmbus_radio
} // namespace esphome
