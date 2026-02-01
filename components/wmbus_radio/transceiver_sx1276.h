#pragma once
#include "transceiver.h"

namespace esphome {
namespace wmbus_radio {
class SX1276 : public RadioTransceiver {
public:
  void setup() override;
  bool get_frame(uint8_t *buffer, size_t length, uint32_t offset) override;
  bool uses_fifo_reading() override { return true; }
  gpio::InterruptType get_interrupt_type() override { return gpio::INTERRUPT_FALLING_EDGE; }
  void restart_rx() override;
  int8_t get_rssi() override;
  const char *get_name() override;

protected:
  optional<uint8_t> read() override;
};
} // namespace wmbus_radio
} // namespace esphome
