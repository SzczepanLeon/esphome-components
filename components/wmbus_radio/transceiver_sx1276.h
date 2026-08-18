#pragma once
#include "transceiver.h"

namespace esphome {
namespace wmbus_radio {
class SX1276 : public RadioTransceiver {
public:
  void setup() override;
  size_t get_frame(uint8_t *buffer, size_t length, uint32_t offset) override;
  // DIO1 is mapped to the packet-ready event; the ISR must react to the rising
  // edge when RX_DONE occurs rather than the falling edge of a stale FIFO-empty
  // condition.
  gpio::InterruptType get_interrupt_type() override { return gpio::INTERRUPT_RISING_EDGE; }
  void restart_rx() override;
  int8_t get_rssi() override;
  const char *get_name() override;

protected:
  optional<uint8_t> read() override;
  uint8_t signal_rssi_{0};
  bool signal_rssi_valid_{false};
};
} // namespace wmbus_radio
} // namespace esphome
