#include "transceiver_sx1276.h"

#include <cstring>
#include "esp_timer.h"
#include "esphome/core/log.h"

#define F_OSC (32000000)

namespace esphome {
namespace wmbus_radio {
static const char *TAG = "SX1276";

void SX1276::setup() {
  this->common_setup();

  ESP_LOGV(TAG, "Setup");
  ESP_LOGVV(TAG, "reset");
  this->reset();

  ESP_LOGVV(TAG, "checking silicon revision");
  uint8_t revision = this->spi_read(0x42);
  ESP_LOGVV(TAG, "revision: %02X", revision);
  if (revision < 0x11 || revision > 0x13) {
    ESP_LOGE(TAG, "Invalid silicon revision: %02X", revision);
    return;
  }

  ESP_LOGVV(TAG, "setting radio frequency");
  const uint32_t frequency = 868950000;

  uint32_t frf = ((uint64_t)frequency * (1 << 19)) / F_OSC;
  this->spi_write(0x06, {BYTE(frf, 2), BYTE(frf, 1), BYTE(frf, 0)});

  // RxBw 250 kHz (Mant=16, Exp=1), AfcBw 333 kHz (Mant=24, Exp=0)
  ESP_LOGVV(TAG, "setting radio bandwidth");
  this->spi_write(0x12, {0x09, 0x10});

  ESP_LOGVV(TAG, "enable AFC auto-clear");
  this->spi_write(0x1A, (uint8_t)(1 << 4));

  ESP_LOGVV(TAG, "set frequency deviation");
  const uint16_t freq_dev = 50000;
  uint16_t frd = ((uint64_t)freq_dev * (1 << 19)) / F_OSC;
  this->spi_write(0x04, {BYTE(frd, 1), BYTE(frd, 0)});

  ESP_LOGVV(TAG, "set bitrate");
  const uint32_t bitrate = 100000;
  uint32_t br = (F_OSC << 4) / bitrate;
  // Fractional part of the bitrate
  this->spi_write(0x5D, (uint8_t)(br & 0x0F));
  br >>= 4;
  // Integer part of the bitrate
  this->spi_write(0x02, {BYTE(br, 1), BYTE(br, 0)});

  ESP_LOGVV(TAG, "set preamble length");
  uint16_t preamble_length = 32 / 8;
  this->spi_write(0x25, {BYTE(preamble_length, 1), BYTE(preamble_length, 0)});

  ESP_LOGVV(TAG, "enable preamble detection");
  uint8_t preamble_detection = (1 << 7) | (1 << 5) | 0x0A;
  this->spi_write(0x1F, preamble_detection);

  ESP_LOGVV(TAG, "enable auto agc/afc");
  uint8_t agc_afc = (1 << 4) | (1 << 3) | 0b110;
  this->spi_write(0x0D, agc_afc);

  ESP_LOGVV(TAG, "disable clock output");
  uint8_t clock_output = 0b111;
  this->spi_write(0x24, clock_output);

  ESP_LOGVV(TAG, "set sync word and reverse preamble polarity");
  uint8_t reverse_preamble_sync_bytes = (1 << 5) | (1 << 4) | (2 - 1);
  this->spi_write(0x27, {reverse_preamble_sync_bytes, 0x54, 0x3D});

  ESP_LOGVV(TAG, "disable crc check/fixed packet length");
  uint8_t crc_check = 0;
  this->spi_write(0x30, crc_check);

  ESP_LOGVV(TAG, "set unlimited packet mode/zero length");
  uint8_t packet_mode = 0;
  this->spi_write(0x32, packet_mode);

  ESP_LOGVV(TAG, "set fifo empty flag on DIO1");
  uint8_t fifo_empty_flag = 0b01 << 4;
  this->spi_write(0x40, fifo_empty_flag);

  ESP_LOGVV(TAG, "set RRSI smoothing");
  uint8_t rssi_smoothing = 0b111;
  this->spi_write(0x0E, rssi_smoothing);

  ESP_LOGVV(TAG, "restart RX");
  this->restart_rx();

  ESP_LOGV(TAG, "SX1276 setup done");
}

optional<uint8_t> SX1276::read() {
  // Read single byte from FIFO if data available (DIO1 low = FIFO not empty)
  if (this->irq_pin_->digital_read() == false)
    return this->spi_read(0x00);

  return {};
}

size_t SX1276::get_frame(uint8_t *buffer, size_t length, uint32_t offset) {
  if (this->irq_pin_->digital_read())
    return 0;

  // Timer-paced batch reads: at 100 kbps each byte arrives every 80 us.
  // DIO1 polling only for the first byte; subsequent batches rely on timing.
  static const size_t BATCH = 32;
  this->delegate_->begin_transaction();
  this->cs_->digital_write(true);

  uint32_t t0 = (uint32_t) esp_timer_get_time();
  size_t count = 0;
  while (count < length) {
    size_t batch = length - count;
    if (batch > BATCH)
      batch = BATCH;

    // Wait until enough bytes have arrived in the FIFO
    uint32_t target = t0 + (count + batch - 1) * 80;
    uint32_t now;
    while ((now = (uint32_t) esp_timer_get_time()) - t0 < target - t0)
      ;

    // SPI transfer: address byte (0x00) + data bytes
    static uint8_t txbuf[BATCH + 1];
    static uint8_t rxbuf[BATCH + 1];
    memset(txbuf, 0, 1 + batch);
    this->cs_->digital_write(false);
    this->delegate_->transfer(txbuf, rxbuf, 1 + batch);
    this->cs_->digital_write(true);

    for (size_t i = 0; i < batch; i++)
      buffer[count++] = rxbuf[1 + i];
  }

  this->delegate_->end_transaction();

  // Capture RSSI while signal is still present
  if (count > 0 && offset == 3 && this->signal_rssi_ == 0)
    this->signal_rssi_ = this->spi_read(0x11);

  return count;
}

void SX1276::restart_rx() {
  // Standby mode
  this->spi_write(0x01, (uint8_t)0b001);
  delay(5);

  // Clear FIFO
  this->spi_write(0x3F, (uint8_t)(1 << 4));

  // Enable RX
  this->spi_write(0x01, (uint8_t)0b101);
  delay(5);
}

int8_t SX1276::get_rssi() {
  uint8_t rssi_now = this->spi_read(0x11);
  // Prefer signal RSSI captured during reception over current (noise floor)
  uint8_t rssi = this->signal_rssi_ ? this->signal_rssi_ : rssi_now;
  this->signal_rssi_ = 0;

  // Clear FIFO overflow if set
  uint8_t irq2 = this->spi_read(0x3F);
  if (irq2 & (1 << 4))
    this->spi_write(0x3F, (uint8_t)(1 << 4));

  return (int8_t)(-rssi / 2);
}

const char *SX1276::get_name() { return TAG; }
} // namespace wmbus_radio
} // namespace esphome
