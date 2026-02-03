#pragma once
#include "esphome/components/spi/spi.h"
#include "esphome/core/gpio.h"
#include "esphome/core/optional.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <string>

#define BYTE(x, n) ((uint8_t)(x >> (n * 8)))

namespace esphome {
namespace wmbus_radio {

// RX Gain modes for SX1262
enum RxGainMode {
  RX_GAIN_BOOSTED,      // Better sensitivity, higher current
  RX_GAIN_POWER_SAVING  // Lower current, reduced sensitivity
};

// Sync modes for SX1262 packet detection
enum SyncMode {
  SYNC_MODE_NORMAL,            // Only RX_DONE IRQ (default)
  SYNC_MODE_ULTRA_LOW_LATENCY  // RX_DONE + SYNC_WORD_VALID IRQ (early wake)
};

class RadioTransceiver
    : public Component,
      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                            spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
public:
  virtual void setup() override = 0;
  void dump_config() override;

  template <typename T>
  void attach_data_interrupt(void (*callback)(T *), T *arg) {
    this->irq_pin_->attach_interrupt(callback, arg, this->get_interrupt_type());
  }

  // Returns the interrupt type for this transceiver
  // SX1276: FALLING edge (DIO1 low = FIFO not empty)
  // SX1262: RISING edge (DIO1 high = packet received)
  virtual gpio::InterruptType get_interrupt_type() { return gpio::INTERRUPT_RISING_EDGE; }
  virtual void restart_rx() = 0;
  virtual int8_t get_rssi() = 0;
  virtual const char *get_name() = 0;

  // Frame-based reading interface
  // Returns number of bytes read (0 if no data available yet)
  // SX1276: returns 1 (byte-by-byte FIFO reading)
  // SX1262: returns length (entire buffer at once)
  virtual size_t get_frame(uint8_t *buffer, size_t length, uint32_t offset) { return 0; }

  bool read_in_task(uint8_t *buffer, size_t length, uint32_t offset);

  void set_spi(spi::SPIDelegate *spi);
  void set_reset_pin(GPIOPin *reset_pin);
  void set_irq_pin(InternalGPIOPin *irq_pin);
  void set_busy_pin(GPIOPin *busy_pin);
  void set_rx_gain_mode(const std::string &mode);
  void set_rf_switch(bool enable);
  void set_sync_mode(const std::string &mode);

protected:
  GPIOPin *reset_pin_{nullptr};
  InternalGPIOPin *irq_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};  // Optional, used by SX1262
  RxGainMode rx_gain_mode_{RX_GAIN_BOOSTED};
  bool rf_switch_{false};  // Use DIO2 as RF switch control (SX1262)
  SyncMode sync_mode_{SYNC_MODE_NORMAL};

  // Byte-by-byte reading interface (used by SX1276) - optional, returns empty if not supported
  virtual optional<uint8_t> read() { return {}; }

  void reset();
  void common_setup();

  // Wait for BUSY pin to go low (SX1262 specific, no-op if busy_pin not set)
  bool wait_busy(uint32_t timeout_ms = 100);

  // SPI operations - SX1276 style (register-based)
  uint8_t spi_transaction(uint8_t operation, uint8_t address,
                          std::initializer_list<uint8_t> data);
  // SPI operations - SX1262 style (command-based)
  uint8_t spi_command(uint8_t command, std::initializer_list<uint8_t> data);
  void spi_read_frame(uint8_t command, std::initializer_list<uint8_t> data,
                      uint8_t *buffer, size_t length);
  uint8_t spi_read(uint8_t address);
  void spi_write(uint8_t address, std::initializer_list<uint8_t> data);
  void spi_write(uint8_t address, uint8_t data);
};

} // namespace wmbus_radio
} // namespace esphome
