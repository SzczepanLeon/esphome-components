#pragma once

#include "cc1101_driver.h"
#include "transceiver.h"
#include <memory>
#include <vector>

namespace esphome {
namespace wmbus_radio {

enum class CC1101State : uint8_t {
  SLEEP = 0x00,
  IDLE = 0x01,
  RX = 0x0D,
  RX_OVERFLOW = 0x11,
  TX = 0x13,
  TX_UNDERFLOW = 0x16,
};

enum class RxLoopState : uint8_t {
  INIT_RX = 0,
  WAIT_FOR_SYNC = 1,
  WAIT_FOR_DATA = 2,
  READ_DATA = 3,
  FRAME_READY = 4,
};

enum class LengthMode : uint8_t {
  INFINITE = 0,
  FIXED = 1,
};

enum class WMBusMode : char {
  MODE_T = 'T',
  MODE_C = 'C',
  UNKNOWN = '?'
};

enum class WMBusBlock : char {
  BLOCK_A = 'A',
  BLOCK_B = 'B',
  UNKNOWN = '?'
};

class CC1101 : public RadioTransceiver {
public:
  CC1101();
  void setup() override;
  void restart_rx() override;
  void run_receiver() override;
  int8_t get_rssi() override;
  const char *get_name() override;
  bool is_frame_oriented() const override;
  void set_gdo0_pin(InternalGPIOPin *pin);
  void set_gdo2_pin(InternalGPIOPin *pin);
  void set_frequency(float freq_mhz);

protected:
  optional<uint8_t> read() override;

private:
  void init_rx_();
  bool wait_for_sync_();
  bool wait_for_data_();
  bool read_data_();
  void set_idle_();
  bool check_rx_overflow_();

  std::unique_ptr<CC1101Driver> driver_;
  InternalGPIOPin *gdo0_pin_;
  InternalGPIOPin *gdo2_pin_;
  float frequency_mhz_;
  RxLoopState rx_state_;
  std::vector<uint8_t> rx_buffer_;
  size_t rx_read_index_;
  size_t bytes_received_;
  size_t expected_length_;
  uint8_t length_field_;
  LengthMode length_mode_;
  WMBusMode wmbus_mode_;
  WMBusBlock wmbus_block_;
  uint32_t sync_time_;
  uint32_t max_wait_time_;
  static constexpr uint8_t WMBUS_MODE_C_PREAMBLE = 0x54;
  static constexpr uint8_t WMBUS_BLOCK_A_PREAMBLE = 0xCD;
  static constexpr uint8_t WMBUS_BLOCK_B_PREAMBLE = 0x3D;
  static constexpr uint8_t RX_FIFO_THRESHOLD = 10;
  static constexpr size_t MAX_FIXED_LENGTH = 256;
  static constexpr size_t MAX_FRAME_SIZE = 512;
};

}
}
