#include "cc1101_driver.h"
#include "esphome/core/log.h"

namespace esphome {
namespace wmbus_radio {

static const char *const TAG = "cc1101_driver";

uint8_t CC1101Driver::read_register(CC1101Register reg) {
  uint8_t addr = static_cast<uint8_t>(reg) | CC1101_READ_SINGLE;
  uint8_t value = 0;

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  value = this->spi_->transfer_byte(0x00);
  this->spi_->disable();

  return value;
}

void CC1101Driver::write_register(CC1101Register reg, uint8_t value) {
  uint8_t addr = static_cast<uint8_t>(reg);

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  this->spi_->transfer_byte(value);
  this->spi_->disable();
}

uint8_t CC1101Driver::read_status(CC1101Status status) {
  uint8_t addr = static_cast<uint8_t>(status) | CC1101_READ_BURST;
  uint8_t value = 0;

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  value = this->spi_->transfer_byte(0x00);
  this->spi_->disable();

  return value;
}

void CC1101Driver::read_burst(CC1101Register reg, uint8_t *buffer,
                               size_t length) {
  if (length == 0)
    return;

  uint8_t addr = static_cast<uint8_t>(reg) | CC1101_READ_BURST;

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  this->spi_->transfer_array(buffer, length);
  this->spi_->disable();
}

void CC1101Driver::write_burst(CC1101Register reg, const uint8_t *buffer,
                                size_t length) {
  if (length == 0)
    return;

  uint8_t addr = static_cast<uint8_t>(reg) | CC1101_WRITE_BURST;

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  for (size_t i = 0; i < length; i++) {
    this->spi_->transfer_byte(buffer[i]);
  }
  this->spi_->disable();
}

uint8_t CC1101Driver::send_strobe(CC1101Strobe strobe) {
  uint8_t addr = static_cast<uint8_t>(strobe);
  uint8_t status = 0;

  this->spi_->enable();
  status = this->spi_->transfer_byte(addr);
  this->spi_->disable();

  return status;
}

void CC1101Driver::read_rx_fifo(uint8_t *buffer, size_t length) {
  if (length == 0)
    return;

  uint8_t addr = CC1101_FIFO | CC1101_READ_BURST;

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  this->spi_->transfer_array(buffer, length);
  this->spi_->disable();
}

void CC1101Driver::write_tx_fifo(const uint8_t *buffer, size_t length) {
  if (length == 0)
    return;

  uint8_t addr = CC1101_FIFO | CC1101_WRITE_BURST;

  this->spi_->enable();
  this->spi_->transfer_byte(addr);
  for (size_t i = 0; i < length; i++) {
    this->spi_->transfer_byte(buffer[i]);
  }
  this->spi_->disable();
}

} // namespace wmbus_radio
} // namespace esphome
