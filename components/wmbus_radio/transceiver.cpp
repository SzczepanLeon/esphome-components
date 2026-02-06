#include "transceiver.h"

#include "esphome/core/log.h"

#include "freertos/FreeRTOS.h"

namespace esphome {
namespace wmbus_radio {
static const char *TAG = "wmbus.transceiver";

bool RadioTransceiver::read_in_task(uint8_t *buffer, size_t length, uint32_t offset) {
  size_t total = 0;
  while (total < length) {
    size_t got = this->get_frame(buffer + total, length - total, offset + total);
    if (got > 0) {
      total += got;
    } else {
      if (!ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)))
        return false;
    }
  }
  return true;
}

void RadioTransceiver::set_reset_pin(GPIOPin *reset_pin) {
  this->reset_pin_ = reset_pin;
}

void RadioTransceiver::set_irq_pin(InternalGPIOPin *irq_pin) {
  this->irq_pin_ = irq_pin;
}

void RadioTransceiver::set_busy_pin(GPIOPin *busy_pin) {
  this->busy_pin_ = busy_pin;
}

void RadioTransceiver::set_rx_gain_mode(const std::string &mode) {
  if (mode == "RX_GAIN_BOOSTED") {
    this->rx_gain_mode_ = RX_GAIN_BOOSTED;
  } else if (mode == "RX_GAIN_POWER_SAVING") {
    this->rx_gain_mode_ = RX_GAIN_POWER_SAVING;
  }
}

void RadioTransceiver::set_rf_switch(bool enable) {
  this->rf_switch_ = enable;
}

void RadioTransceiver::set_sync_mode(const std::string &mode) {
  if (mode == "SYNC_MODE_NORMAL") {
    this->sync_mode_ = SYNC_MODE_NORMAL;
  } else if (mode == "SYNC_MODE_ULTRA_LOW_LATENCY") {
    this->sync_mode_ = SYNC_MODE_ULTRA_LOW_LATENCY;
  }
}

void RadioTransceiver::set_tcxo(bool enable) {
  this->has_tcxo_ = enable;
}

bool RadioTransceiver::wait_busy(uint32_t timeout_ms) {
  if (this->busy_pin_ == nullptr) {
    return true;  // No BUSY pin configured, assume ready
  }

  uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > timeout_ms) {
      ESP_LOGE(TAG, "BUSY pin timeout after %u ms", timeout_ms);
      this->mark_failed();
      return false;
    }
    delayMicroseconds(100);  // Small delay to avoid busy-spinning
  }
  return true;
}

void RadioTransceiver::reset() {
  this->reset_pin_->digital_write(0);
  delay(5);
  this->reset_pin_->digital_write(1);
  delay(5);

  // Wait for BUSY to go low after reset (no-op if busy_pin not configured)
  this->wait_busy(100);
}

void RadioTransceiver::common_setup() {
  this->reset_pin_->setup();
  this->irq_pin_->setup();
  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();
  }
  this->spi_setup();
}

// SX1276-style SPI transaction (register-based with operation | address)
uint8_t RadioTransceiver::spi_transaction(uint8_t operation, uint8_t address,
                                          std::initializer_list<uint8_t> data) {
  this->delegate_->begin_transaction();
  auto rval = this->delegate_->transfer(operation | address);
  for (auto byte : data)
    rval = this->delegate_->transfer(byte);
  this->delegate_->end_transaction();
  return rval;
}

// SX1262-style SPI command (command-based, waits for BUSY)
uint8_t RadioTransceiver::spi_command(uint8_t command,
                                      std::initializer_list<uint8_t> data) {
  this->wait_busy();

  this->delegate_->begin_transaction();
  auto rval = this->delegate_->transfer(command);
  for (auto byte : data)
    rval = this->delegate_->transfer(byte);
  this->delegate_->end_transaction();
  return rval;
}

void RadioTransceiver::spi_read_frame(uint8_t command,
                                      std::initializer_list<uint8_t> data,
                                      uint8_t *buffer, size_t length) {
  this->wait_busy();

  this->delegate_->begin_transaction();
  this->delegate_->transfer(command);
  for (auto byte : data)
    this->delegate_->transfer(byte);
  for (size_t i = 0; i < length; i++)
    *buffer++ = this->delegate_->transfer(0x55);
  this->delegate_->end_transaction();
}

uint8_t RadioTransceiver::spi_read(uint8_t address) {
  return this->spi_transaction(0x00, address, {0});
}

void RadioTransceiver::spi_write(uint8_t address,
                                 std::initializer_list<uint8_t> data) {
  this->spi_transaction(0x80, address, data);
}

void RadioTransceiver::spi_write(uint8_t address, uint8_t data) {
  this->spi_write(address, {data});
}

void RadioTransceiver::dump_config() {
  ESP_LOGCONFIG(TAG, "Transceiver: %s", this->get_name());
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  IRQ Pin: ", this->irq_pin_);
  if (this->busy_pin_ != nullptr) {
    LOG_PIN("  BUSY Pin: ", this->busy_pin_);
  }
  ESP_LOGCONFIG(TAG, "  RX Gain: %s",
                this->rx_gain_mode_ == RX_GAIN_BOOSTED ? "Boosted" : "Power Saving");
  if (this->rf_switch_) {
    ESP_LOGCONFIG(TAG, "  RF Switch: DIO2");
  }
  ESP_LOGCONFIG(TAG, "  Sync Mode: %s",
                this->sync_mode_ == SYNC_MODE_ULTRA_LOW_LATENCY ? "Ultra Low Latency" : "Normal");
  if (this->has_tcxo_) {
    ESP_LOGCONFIG(TAG, "  TCXO: DIO3");
  }
}
} // namespace wmbus_radio
} // namespace esphome
