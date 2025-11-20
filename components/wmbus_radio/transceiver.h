#pragma once
#include "esphome/components/spi/spi.h"
#include "esphome/core/gpio.h"
#include "esphome/core/optional.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <cstdint>

#define BYTE(x, n) ((uint8_t)(x >> (n * 8)))

namespace esphome {
namespace wmbus_radio {
class RadioTransceiver
    : public Component,
      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                            spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_2MHZ> {
public:
  RadioTransceiver();

  virtual void setup() override = 0;
  void dump_config() override;

  template <typename T>
  void attach_data_interrupt(void (*callback)(T *), T *arg) {
    this->irq_pin_->attach_interrupt(callback, arg, this->irq_interrupt_type());
  }

  bool has_irq_pin() const;

  virtual void restart_rx() = 0;
  virtual int8_t get_rssi() = 0;
  virtual const char *get_name() = 0;

  virtual bool is_frame_oriented() const;
  virtual void run_receiver();

  bool read_in_task(uint8_t *buffer, size_t length);

  void set_spi(spi::SPIDelegate *spi);
  void set_reset_pin(InternalGPIOPin *reset_pin);
  void set_irq_pin(InternalGPIOPin *irq_pin);
  void set_polling_interval(uint32_t interval_ms);
  uint32_t get_polling_interval() const;
  void set_packet_queue(QueueHandle_t queue);
  virtual gpio::InterruptType irq_interrupt_type() const;

protected:
  InternalGPIOPin *reset_pin_;
  InternalGPIOPin *irq_pin_;
  uint32_t polling_interval_ms_;
  QueueHandle_t packet_queue_;

  virtual optional<uint8_t> read() = 0;

  void reset();
  void common_setup();
  uint8_t spi_transaction(uint8_t operation, uint8_t address,
                          std::initializer_list<uint8_t> data);
  uint8_t spi_read(uint8_t address);
  void spi_write(uint8_t address, std::initializer_list<uint8_t> data);
  void spi_write(uint8_t address, uint8_t data);
};

} // namespace wmbus_radio
} // namespace esphome
