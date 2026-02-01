#include "transceiver_sx1262.h"

#include "esphome/core/log.h"

namespace esphome {
namespace wmbus_radio {
static const char *TAG = "SX1262";

void SX1262::setup() {
  this->common_setup();

  ESP_LOGV(TAG, "Setup");
  ESP_LOGVV(TAG, "reset");
  this->reset();

  ESP_LOGVV(TAG, "checking silicon revision");
  uint32_t revision = 0x1262; // this->spi_read(0x42);
  ESP_LOGVV(TAG, "revision: %04X", revision);
  if (revision != 0x1262) {
    ESP_LOGE(TAG, "Invalid silicon revision: %04X", revision);
    return;
  }

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
  uint8_t rx_gain_val = (this->rx_gain_mode_ == RX_GAIN_BOOSTED) ? 0x96 : 0x94;
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
  const uint32_t irqmask = RADIOLIB_SX126X_IRQ_RX_DONE;
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

  ESP_LOGVV(TAG, "setting DIO3 as TCXO control");
  const uint32_t tcxodelay = 64;
  this->spi_command(RADIOLIB_SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, {
                    RADIOLIB_SX126X_DIO3_OUTPUT_3_0,
                    BYTE(tcxodelay, 2), BYTE(tcxodelay, 1), BYTE(tcxodelay, 0)
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

  this->offset = 0;

  ESP_LOGV(TAG, "SX1262 setup done");
}

bool SX1262::get_frame(uint8_t *buffer, size_t length, uint32_t offset) {
  if (this->irq_pin_->digital_read()) {
    spi_read_frame(RADIOLIB_SX126X_CMD_READ_BUFFER, {uint8_t(offset), 0x00}, buffer, length);

    // Clear IRQ
    if (offset > 0) {
      const uint32_t irqmask = RADIOLIB_SX126X_IRQ_RX_DONE;
      this->spi_command(RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS, {BYTE(irqmask, 1), BYTE(irqmask, 0)});

      const uint32_t timeout = 0x000000; // 0xFFFFFF;
      this->spi_command(RADIOLIB_SX126X_CMD_SET_RX, {
                        BYTE(timeout, 2), BYTE(timeout, 1), BYTE(timeout, 0)
      });
    }
    return true;
  }

  return false;
}

void SX1262::restart_rx() {
  ESP_LOGVV(TAG, "Restarting RX");
  // Standby mode
  this->spi_command(RADIOLIB_SX126X_CMD_SET_STANDBY, {RADIOLIB_SX126X_STANDBY_XOSC});
  delay(5);

  // Clear IRQ
  const uint32_t irqmask = RADIOLIB_SX126X_IRQ_RX_DONE;
  this->spi_command(RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS, {
                    BYTE(irqmask, 1), BYTE(irqmask, 0)
  });

  // Enable RX
  const uint32_t timeout = 0x000000;
  this->spi_command(RADIOLIB_SX126X_CMD_SET_RX, {
                    BYTE(timeout, 2), BYTE(timeout, 1), BYTE(timeout, 0)
  });
  delay(5);

  // Start reading at buffer offset
  this->offset = 0;
}

int8_t SX1262::get_rssi() {
  uint8_t rssi = this->spi_command(RADIOLIB_SX126X_CMD_GET_PACKET_STATUS, {0x00, 0x00, 0x00});
  return (int8_t)(-rssi / 2);
}

const char *SX1262::get_name() { return TAG; }
} // namespace wmbus_radio
} // namespace esphome
