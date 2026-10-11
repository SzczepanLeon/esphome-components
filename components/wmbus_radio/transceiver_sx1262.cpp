#include "transceiver_sx1262.h"

#include "esphome/core/log.h"

#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome {
namespace wmbus_radio {
static const char *TAG = "SX1262";

void SX1262::setup() {
  this->common_setup();

  ESP_LOGV(TAG, "Setup");
  ESP_LOGVV(TAG, "reset");
  this->reset();

  ESP_LOGVV(TAG, "setting Standby mode");
  this->spi_command(RADIOLIB_SX126X_CMD_SET_STANDBY, {RADIOLIB_SX126X_STANDBY_RC});

  ESP_LOGVV(TAG, "setting packet type");
  this->spi_command(RADIOLIB_SX126X_CMD_SET_PACKET_TYPE, {RADIOLIB_SX126X_PACKET_TYPE_GFSK});

  ESP_LOGVV(TAG, "setting radio frequency");
  const float frequency = 868.950;
  const uint32_t frf = (frequency * (uint32_t(1) << RADIOLIB_SX126X_DIV_EXPONENT)) / RADIOLIB_SX126X_CRYSTAL_FREQ;
  this->spi_command(RADIOLIB_SX126X_CMD_SET_RF_FREQUENCY, {
                    BYTE(frf, 3), BYTE(frf, 2), BYTE(frf, 1), BYTE(frf, 0)});

  ESP_LOGVV(TAG, "setting buffer base address");
  this->spi_command(RADIOLIB_SX126X_CMD_SET_BUFFER_BASE_ADDRESS, {0x00, 0x00});

  ESP_LOGVV(TAG, "setting modulation parameters");
  uint32_t bitrate = (uint32_t)((RADIOLIB_SX126X_CRYSTAL_FREQ * 1000000.0f * 32.0f) / (100.0f * 1000.0f));
  uint32_t freqdev = (uint32_t)(((50.0f * 1000.0f) * (float)((uint32_t)(1) << 25)) / (RADIOLIB_SX126X_CRYSTAL_FREQ * 1000000.0f));
  this->spi_command(RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS, {
                    BYTE(bitrate, 2), BYTE(bitrate, 1), BYTE(bitrate, 0),
                    RADIOLIB_SX126X_GFSK_FILTER_NONE,
                    RADIOLIB_SX126X_GFSK_RX_BW_234_3,
                    BYTE(freqdev, 2), BYTE(freqdev, 1), BYTE(freqdev, 0)
  });

  ESP_LOGVV(TAG, "setting packet parameters");
  this->spi_command(RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS, {
                    BYTE(16, 1), BYTE(16, 0),   // Preamble length
                    RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_8,
                    16,                         // Sync word bit length
                    RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF,
                    RADIOLIB_SX126X_GFSK_PACKET_FIXED,
                    0xff,                       // Payload length
                    RADIOLIB_SX126X_GFSK_CRC_OFF,
                    RADIOLIB_SX126X_GFSK_WHITENING_OFF
  });

  ESP_LOGVV(TAG, "setting RX gain");
  uint8_t rx_gain_val = (this->rx_gain_mode_ == RX_GAIN_BOOSTED)
                            ? RADIOLIB_SX126X_RX_GAIN_BOOSTED
                            : RADIOLIB_SX126X_RX_GAIN_POWER_SAVING;
  this->spi_command(RADIOLIB_SX126X_CMD_WRITE_REGISTER, {
                    BYTE(RADIOLIB_SX126X_REG_RX_GAIN, 1), BYTE(RADIOLIB_SX126X_REG_RX_GAIN, 0),
                    rx_gain_val
  });

  // Configure DIO2 as RF switch control if enabled
  if (this->rf_switch_) {
    ESP_LOGVV(TAG, "setting DIO2 as RF switch control");
    this->spi_command(RADIOLIB_SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, {0x01});
  }

  ESP_LOGVV(TAG, "setting IRQ parameters");
  // Trigger on sync word so the payload can be streamed out of the 256-byte
  // buffer while it is still being received (frames longer than 255 bytes).
  const uint32_t irqmask = RADIOLIB_SX126X_IRQ_SYNC_WORD_VALID;
  this->spi_command(RADIOLIB_SX126X_CMD_SET_DIO_IRQ_PARAMS, {
                    BYTE(irqmask, 1), BYTE(irqmask, 0),
                    BYTE(irqmask, 1), BYTE(irqmask, 0),
                    0x00, 0x00,
                    0x00, 0x00
  });

  ESP_LOGVV(TAG, "setting sync word");
  this->spi_command(RADIOLIB_SX126X_CMD_WRITE_REGISTER, {
                    BYTE(RADIOLIB_SX126X_REG_SYNC_WORD_0, 1), BYTE(RADIOLIB_SX126X_REG_SYNC_WORD_0, 0),
                    0x54, 0x3d, 0x00, 0x00, 0x00, 0x00
  });

  // Configure DIO2 as a TCXO driver
  if (this->has_tcxo_) {
    ESP_LOGVV(TAG, "setting DIO3 as TCXO control");
    const uint32_t tcxodelay = 64;
    this->spi_command(RADIOLIB_SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, {
                      RADIOLIB_SX126X_DIO3_OUTPUT_3_0,
                      BYTE(tcxodelay, 2), BYTE(tcxodelay, 1), BYTE(tcxodelay, 0)
    });
  }

  ESP_LOGVV(TAG, "running calibration");
  this->spi_command(RADIOLIB_SX126X_CMD_CALIBRATE, {
                    RADIOLIB_SX126X_CALIBRATE_ALL & ~RADIOLIB_SX126X_CALIBRATE_IMAGE_ON
  });

  ESP_LOGVV(TAG, "image calibration for 868MHz");
  this->spi_command(RADIOLIB_SX126X_CMD_CALIBRATE_IMAGE, {
                    RADIOLIB_SX126X_CAL_IMG_863_MHZ_1, RADIOLIB_SX126X_CAL_IMG_863_MHZ_2
  });

  ESP_LOGVV(TAG, "setting fallback mode");
  this->spi_command(RADIOLIB_SX126X_CMD_SET_RX_TX_FALLBACK_MODE, {RADIOLIB_SX126X_RX_TX_FALLBACK_MODE_STDBY_XOSC});

  ESP_LOGVV(TAG, "setting Standby mode");
  this->spi_command(RADIOLIB_SX126X_CMD_SET_STANDBY, {RADIOLIB_SX126X_STANDBY_XOSC});

  ESP_LOGVV(TAG, "setting RX mode");
  const uint32_t timeout = 0x000000; // 0xFFFFFF;
  this->spi_command(RADIOLIB_SX126X_CMD_SET_RX, {
                    BYTE(timeout, 2), BYTE(timeout, 1), BYTE(timeout, 0)
  });

  ESP_LOGV(TAG, "SX1262 setup done");
}

// Long packet reception (Semtech AN1200.53).
// In fixed length mode the radio stops after REG_RX_TX_PLD_LEN bytes (max 255),
// but T1 telegrams can be longer after 3of6 encoding. While receiving, the
// payload length register is kept just behind the write pointer, so the end of
// the packet is never reached and data is read from the circular buffer as it
// arrives. Reception is restarted by the caller once the frame is complete.
bool SX1262::read_in_task(uint8_t *buffer, size_t length, uint32_t offset) {
  // 100 kbps -> 12.5 bytes/ms, allow plenty of margin
  const uint32_t timeout_ms = 20 + length / 8;
  const uint32_t start = millis();
  size_t total = 0;

  while (total < length) {
    const uint8_t write_ptr = this->read_register(RADIOLIB_SX126X_REG_RX_ADDR_PTR);
    this->write_register(RADIOLIB_SX126X_REG_RX_TX_PLD_LEN, uint8_t(write_ptr - 1));

    const uint8_t read_ptr = uint8_t(offset + total);
    size_t avail = uint8_t(write_ptr - read_ptr);
    if (avail == 0) {
      if (millis() - start > timeout_ms) {
        ESP_LOGV(TAG, "RX stream timeout (%zu/%zu bytes at offset %u)", total, length, (unsigned) offset);
        return false;
      }
      vTaskDelay(1);
      continue;
    }

    avail = std::min(avail, length - total);
    // Read up to the end of the buffer, the remainder wraps to address 0
    const size_t first = std::min<size_t>(avail, 256 - read_ptr);
    this->spi_read_frame(RADIOLIB_SX126X_CMD_READ_BUFFER, {read_ptr, 0x00}, buffer + total, first);
    if (avail > first)
      this->spi_read_frame(RADIOLIB_SX126X_CMD_READ_BUFFER, {0x00, 0x00}, buffer + total + first, avail - first);
    total += avail;
  }

  return true;
}

void SX1262::restart_rx() {
  ESP_LOGVV(TAG, "Restarting RX");
  // Standby mode
  this->spi_command(RADIOLIB_SX126X_CMD_SET_STANDBY, {RADIOLIB_SX126X_STANDBY_XOSC});
  delay(5);

  // Clear all IRQs
  const uint32_t irqmask = RADIOLIB_SX126X_IRQ_ALL;
  this->spi_command(RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS, {
                    BYTE(irqmask, 1), BYTE(irqmask, 0)
  });

  // Restore payload length modified by long packet reception
  this->write_register(RADIOLIB_SX126X_REG_RX_TX_PLD_LEN, 0xff);

  // Enable RX
  const uint32_t timeout = 0x000000;
  this->spi_command(RADIOLIB_SX126X_CMD_SET_RX, {
                    BYTE(timeout, 2), BYTE(timeout, 1), BYTE(timeout, 0)
  });
  delay(5);
}

int8_t SX1262::get_rssi() {
  uint8_t rssi = this->spi_command(RADIOLIB_SX126X_CMD_GET_PACKET_STATUS, {0x00, 0x00, 0x00});
  return (int8_t)(-rssi / 2);
}

const char *SX1262::get_name() { return TAG; }

uint16_t SX1262::get_irq_status() {
  uint8_t status[3] = {0};
  this->wait_busy();
  this->delegate_->begin_transaction();
  this->delegate_->transfer(RADIOLIB_SX126X_CMD_GET_IRQ_STATUS);
  status[0] = this->delegate_->transfer(0x00);  // NOP
  status[1] = this->delegate_->transfer(0x00);  // IRQ status MSB
  status[2] = this->delegate_->transfer(0x00);  // IRQ status LSB
  this->delegate_->end_transaction();
  return (status[1] << 8) | status[2];
}

uint8_t SX1262::read_register(uint16_t address) {
  uint8_t value = 0;
  this->spi_read_frame(RADIOLIB_SX126X_CMD_READ_REGISTER, {BYTE(address, 1), BYTE(address, 0), 0x00}, &value, 1);
  return value;
}

void SX1262::write_register(uint16_t address, uint8_t value) {
  this->spi_command(RADIOLIB_SX126X_CMD_WRITE_REGISTER, {BYTE(address, 1), BYTE(address, 0), value});
}
} // namespace wmbus_radio
} // namespace esphome
