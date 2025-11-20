#include "transceiver_cc1101.h"
#include "cc1101_rf_settings.h"
#include "decode3of6.h"
#include "packet.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome {
namespace wmbus_radio {

static const char *const TAG = "cc1101";

CC1101::CC1101()
    : gdo0_pin_(nullptr)
    , gdo2_pin_(nullptr)
    , frequency_mhz_(868.95f)
    , rx_state_(RxLoopState::INIT_RX)
    , rx_read_index_(0)
    , bytes_received_(0)
    , expected_length_(0)
    , length_field_(0)
    , length_mode_(LengthMode::INFINITE)
    , wmbus_mode_(WMBusMode::UNKNOWN)
    , wmbus_block_(WMBusBlock::UNKNOWN)
    , sync_time_(0)
    , max_wait_time_(150) {}

const char *CC1101::get_name() {
  return "CC1101";
}

bool CC1101::is_frame_oriented() const {
  return true;
}

void CC1101::set_gdo0_pin(InternalGPIOPin *pin) {
  this->gdo0_pin_ = pin;
}

void CC1101::set_gdo2_pin(InternalGPIOPin *pin) {
  this->gdo2_pin_ = pin;
}

void CC1101::set_frequency(float freq_mhz) {
  this->frequency_mhz_ = freq_mhz;
}
static size_t mode_a_decoded_size(uint8_t l_field) {
  size_t num_blocks = (l_field < 26) ? 2 : ((l_field - 26) / 16 + 3);
  return l_field + 1 + 2 * num_blocks;
}
static size_t mode_t_packet_size(uint8_t l_field) {
  return encoded_size(mode_a_decoded_size(l_field));
}
static size_t mode_c_expected_length(uint8_t l_field, WMBusBlock block_type,
                                     bool has_preamble) {
  size_t base = 0;
  size_t l = l_field;
  if (block_type == WMBusBlock::BLOCK_A) {
    base = mode_a_decoded_size(l_field);
  } else if (block_type == WMBusBlock::BLOCK_B) {
    base = 1 + l;
  } else {
    return 0;
  }
  return has_preamble ? (base + 2) : base;
}
void CC1101::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CC1101...");
  if (this->gdo0_pin_ != nullptr) {
    this->gdo0_pin_->setup();
    this->gdo0_pin_->pin_mode(gpio::FLAG_INPUT);
  }
  if (this->gdo2_pin_ != nullptr) {
    this->gdo2_pin_->setup();
    this->gdo2_pin_->pin_mode(gpio::FLAG_INPUT);
  }
  this->common_setup();
  this->driver_ = std::make_unique<CC1101Driver>(this);
  ESP_LOGD(TAG, "Sending software reset (SRES strobe)...");
  this->driver_->send_strobe(CC1101Strobe::SRES);
  delay(10);
  uint8_t partnum = this->driver_->read_status(CC1101Status::PARTNUM);
  uint8_t version = this->driver_->read_status(CC1101Status::VERSION);
  ESP_LOGD(TAG, "CC1101 PARTNUM: 0x%02X (expected: 0x00)", partnum);
  ESP_LOGD(TAG, "CC1101 VERSION: 0x%02X (expected: 0x04 or 0x14)", version);
  if (version == 0 || version == 0xFF) {
    ESP_LOGE(TAG, "CC1101 not detected! SPI communication failed. Check wiring:");
    ESP_LOGE(TAG, "  - CS pin: connected and correct?");
    ESP_LOGE(TAG, "  - MOSI/MISO/SCK: connected and correct?");
    ESP_LOGE(TAG, "  - VCC: 3.3V supplied?");
    ESP_LOGE(TAG, "  - GND: connected?");
    this->mark_failed();
    return;
  }
  if (partnum != 0x00) {
    ESP_LOGW(TAG, "Unexpected PARTNUM 0x%02X (expected 0x00). Chip may not be CC1101.", partnum);
  }
  ESP_LOGCONFIG(TAG, "CC1101 detected - PARTNUM: 0x%02X, VERSION: 0x%02X", partnum, version);
  ESP_LOGD(TAG, "Applying wM-Bus RF settings (%zu registers)...", CC1101_WMBUS_RF_SETTINGS.size());
  apply_wmbus_rf_settings(*this->driver_);
  uint8_t iocfg2 = this->driver_->read_register(CC1101Register::IOCFG2);
  uint8_t iocfg0 = this->driver_->read_register(CC1101Register::IOCFG0);
  uint8_t sync1 = this->driver_->read_register(CC1101Register::SYNC1);
  uint8_t sync0 = this->driver_->read_register(CC1101Register::SYNC0);
  ESP_LOGD(TAG, "Register verification:");
  ESP_LOGD(TAG, "  IOCFG2 (GDO2 config): 0x%02X (expected: 0x06)", iocfg2);
  ESP_LOGD(TAG, "  IOCFG0 (GDO0 config): 0x%02X (expected: 0x00)", iocfg0);
  ESP_LOGD(TAG, "  SYNC1: 0x%02X (expected: 0x54)", sync1);
  ESP_LOGD(TAG, "  SYNC0: 0x%02X (expected: 0x3D)", sync0);
  bool registers_ok = (iocfg2 == 0x06) && (iocfg0 == 0x00) &&
                      (sync1 == 0x54) && (sync0 == 0x3D);
  if (!registers_ok) {
    ESP_LOGW(TAG, "Register verification failed! SPI communication may be unreliable.");
  } else {
    ESP_LOGD(TAG, "Register verification passed - RF settings applied successfully");
  }
  if (this->frequency_mhz_ != 868.95f) {
    ESP_LOGD(TAG, "Setting custom frequency: %.2f MHz", this->frequency_mhz_);
    set_carrier_frequency(*this->driver_, this->frequency_mhz_);
    uint8_t freq2 = this->driver_->read_register(CC1101Register::FREQ2);
    uint8_t freq1 = this->driver_->read_register(CC1101Register::FREQ1);
    uint8_t freq0 = this->driver_->read_register(CC1101Register::FREQ0);
    uint32_t freq_reg = (static_cast<uint32_t>(freq2) << 16) |
                        (static_cast<uint32_t>(freq1) << 8) |
                        freq0;
    float actual_freq = (freq_reg * 26.0f) / 65536.0f;
    ESP_LOGD(TAG, "Frequency registers: 0x%02X%02X%02X (%.2f MHz)", freq2, freq1, freq0, actual_freq);
  }
  ESP_LOGD(TAG, "Calibrating frequency synthesizer (SCAL strobe)...");
  this->driver_->send_strobe(CC1101Strobe::SCAL);
  delay(4);
  uint8_t marcstate = this->driver_->read_status(CC1101Status::MARCSTATE);
  ESP_LOGD(TAG, "MARCSTATE after calibration: 0x%02X (IDLE=0x01)", marcstate);
  ESP_LOGCONFIG(TAG, "CC1101 initialized successfully");
  ESP_LOGCONFIG(TAG, "  Chip version: 0x%02X", version);
  ESP_LOGCONFIG(TAG, "  Frequency: %.2f MHz", this->frequency_mhz_);
  bool gdo0_initial = (this->gdo0_pin_ != nullptr) ? this->gdo0_pin_->digital_read() : false;
  bool gdo2_initial = (this->gdo2_pin_ != nullptr) ? this->gdo2_pin_->digital_read() : false;
  ESP_LOGD(TAG, "GDO pin initial states: GDO0=%d, GDO2=%d", gdo0_initial, gdo2_initial);
  this->restart_rx();
  delay(5);
  bool gdo0_rx = (this->gdo0_pin_ != nullptr) ? this->gdo0_pin_->digital_read() : false;
  bool gdo2_rx = (this->gdo2_pin_ != nullptr) ? this->gdo2_pin_->digital_read() : false;
  ESP_LOGD(TAG, "GDO pin states in RX mode: GDO0=%d, GDO2=%d", gdo0_rx, gdo2_rx);
  if (gdo0_initial == gdo0_rx && gdo2_initial == gdo2_rx) {
    ESP_LOGW(TAG, "GDO pins did not change state - check pin connections!");
  }
  ESP_LOGCONFIG(TAG, "CC1101 setup complete");
}
void CC1101::restart_rx() {
  this->set_idle_();
  this->init_rx_();
}
void CC1101::run_receiver() {
  RxLoopState state_before;
  do {
    state_before = this->rx_state_;
    this->read();
    if (this->rx_state_ == RxLoopState::FRAME_READY) {
      break;
    }
    if (this->rx_state_ == RxLoopState::WAIT_FOR_SYNC) {
      return;
    }
    if (this->rx_state_ == state_before) {
      return;
    }
  } while (this->rx_state_ != RxLoopState::FRAME_READY);
  if (this->rx_state_ != RxLoopState::FRAME_READY) {
    return;
  }
  auto packet = std::make_unique<Packet>();
  bool requires_decode = true;
  std::vector<uint8_t> frame_data = this->rx_buffer_;
  if (this->wmbus_mode_ == WMBusMode::MODE_T) {
    auto decoded = decode3of6(frame_data);
    if (!decoded.has_value()) {
      ESP_LOGW(TAG, "3-of-6 decode failed");
      this->rx_state_ = RxLoopState::INIT_RX;
      return;
    }
    frame_data = std::move(decoded.value());
    requires_decode = false;
    ESP_LOGD(TAG, "3-of-6 decode successful, decoded to %zu bytes", frame_data.size());
  }
  packet->set_data(frame_data);
  packet->set_requires_decode(requires_decode && this->wmbus_mode_ == WMBusMode::MODE_T);
  if (this->wmbus_mode_ == WMBusMode::MODE_C) {
    packet->set_link_mode_hint(LinkMode::C1);
  } else if (this->wmbus_mode_ == WMBusMode::MODE_T) {
    packet->set_link_mode_hint(LinkMode::T1);
  }
  packet->set_rssi(this->get_rssi());
  this->rx_read_index_ = this->rx_buffer_.size();
  if (!packet->calculate_payload_size()) {
    ESP_LOGD(TAG, "Cannot calculate payload size");
    this->rx_state_ = RxLoopState::INIT_RX;
    return;
  }
  auto packet_ptr = packet.get();
  if (xQueueSend(this->packet_queue_, &packet_ptr, 0) == pdTRUE) {
    ESP_LOGV(TAG, "Frame queued successfully");
    packet.release();
  } else {
    ESP_LOGW(TAG, "Queue send failed");
  }
}
int8_t CC1101::get_rssi() {
  uint8_t rssi_raw = this->driver_->read_status(CC1101Status::RSSI);
  int16_t rssi_dbm;
  if (rssi_raw >= 128) {
    rssi_dbm = ((rssi_raw - 256) / 2) - 74;
  } else {
    rssi_dbm = (rssi_raw / 2) - 74;
  }
  return static_cast<int8_t>(rssi_dbm);
}
optional<uint8_t> CC1101::read() {
  switch (this->rx_state_) {
  case RxLoopState::FRAME_READY:
    if (this->rx_read_index_ < this->rx_buffer_.size()) {
      return this->rx_buffer_[this->rx_read_index_++];
    }
    this->rx_state_ = RxLoopState::INIT_RX;
    return {};
  case RxLoopState::INIT_RX:
    this->init_rx_();
    return {};
  case RxLoopState::WAIT_FOR_SYNC:
    if (this->wait_for_sync_()) {
      ESP_LOGD(TAG, "Sync detected");
      this->rx_state_ = RxLoopState::WAIT_FOR_DATA;
      this->sync_time_ = millis();
      return {};
    }
    {
      uint8_t rxbytes_status = this->driver_->read_status(CC1101Status::RXBYTES);
      if (rxbytes_status & 0x80) {
        ESP_LOGW(TAG, "FIFO overflow while waiting for sync, flushing");
        this->rx_state_ = RxLoopState::INIT_RX;
        return {};
      }
    }
    return {};
    return {};
  case RxLoopState::WAIT_FOR_DATA:
    if (millis() - this->sync_time_ > this->max_wait_time_) {
      ESP_LOGW(TAG, "Timeout waiting for data after sync! Resetting RX.");
      this->rx_state_ = RxLoopState::INIT_RX;
      return {};
    }
    if (this->wait_for_data_()) {
      ESP_LOGD(TAG, "Header received, processing frame data");
      this->rx_state_ = RxLoopState::READ_DATA;
    } else {
      return {};
    }
    [[fallthrough]];
  case RxLoopState::READ_DATA:
    while (true) {
      size_t bytes_before = this->bytes_received_;
      if (this->read_data_()) {
        ESP_LOGI(TAG, "Frame received: %zu bytes, mode: %c, L=0x%02X",
                 this->rx_buffer_.size(),
                 static_cast<char>(this->wmbus_mode_),
                 this->length_field_);
        this->rx_state_ = RxLoopState::FRAME_READY;
        this->rx_read_index_ = 0;
        if (!this->rx_buffer_.empty()) {
          return this->rx_buffer_[this->rx_read_index_++];
        }
        ESP_LOGW(TAG, "RX buffer empty after frame reception");
        this->rx_state_ = RxLoopState::INIT_RX;
        return {};
      }
      if (this->bytes_received_ == bytes_before) {
        break;
      }
    }
    return {};
  default:
    this->rx_state_ = RxLoopState::INIT_RX;
    return {};
  }
}
void CC1101::init_rx_() {
  this->set_idle_();
  this->driver_->send_strobe(CC1101Strobe::SFTX);
  this->driver_->send_strobe(CC1101Strobe::SFRX);
  this->driver_->write_register(CC1101Register::FIFOTHR, 0x0A);
  this->driver_->write_register(CC1101Register::PKTCTRL0, 0x02);
  this->rx_buffer_.clear();
  this->rx_read_index_ = 0;
  this->bytes_received_ = 0;
  this->expected_length_ = 0;
  this->length_field_ = 0;
  this->length_mode_ = LengthMode::INFINITE;
  this->wmbus_mode_ = WMBusMode::UNKNOWN;
  this->wmbus_block_ = WMBusBlock::UNKNOWN;
  this->driver_->send_strobe(CC1101Strobe::SRX);
  uint8_t marc_state;
  bool rx_entered = false;
  for (int i = 0; i < 10; i++) {
    marc_state = this->driver_->read_status(CC1101Status::MARCSTATE);
    if (marc_state == static_cast<uint8_t>(CC1101State::RX)) {
      rx_entered = true;
      break;
    }
    delay(1);
  }
  if (!rx_entered) {
    ESP_LOGW(TAG, "Failed to enter RX mode! MARCSTATE: 0x%02X (expected: 0x0D)", marc_state);
  }
  this->rx_state_ = RxLoopState::WAIT_FOR_SYNC;
}
bool CC1101::wait_for_sync_() {
  if (this->gdo2_pin_ != nullptr) {
    return this->gdo2_pin_->digital_read();
  }
  return false;
}
bool CC1101::wait_for_data_() {
  uint8_t rxbytes_status = this->driver_->read_status(CC1101Status::RXBYTES);
  if (rxbytes_status & 0x80) {
    ESP_LOGW(TAG, "RX FIFO overflow while reading header");
    this->rx_state_ = RxLoopState::INIT_RX;
    return false;
  }
  uint8_t bytes_in_fifo = rxbytes_status & 0x7F;
  if (bytes_in_fifo < 4) {
    return false;
  }
  ESP_LOGD(TAG, "FIFO has %d bytes, reading header", bytes_in_fifo);
  uint8_t header[4];
  this->driver_->read_rx_fifo(header, 4);
  ESP_LOGD(TAG, "Header bytes: %02X %02X %02X %02X", header[0], header[1], header[2], header[3]);
  if (header[0] == WMBUS_MODE_C_PREAMBLE) {
    this->wmbus_mode_ = WMBusMode::MODE_C;
    if (header[1] == WMBUS_BLOCK_A_PREAMBLE) {
      this->wmbus_block_ = WMBusBlock::BLOCK_A;
    } else if (header[1] == WMBUS_BLOCK_B_PREAMBLE) {
      this->wmbus_block_ = WMBusBlock::BLOCK_B;
    } else {
      ESP_LOGV(TAG, "Unknown Mode C block type: 0x%02X", header[1]);
      return false;
    }
    this->length_field_ = header[2];
    this->rx_buffer_.insert(this->rx_buffer_.end(), header, header + 4);
    this->expected_length_ =
        mode_c_expected_length(this->length_field_, this->wmbus_block_, true);
  } else {
    std::vector<uint8_t> header_vec(header, header + 3);
    auto decoded_header = decode3of6(header_vec);
    if (decoded_header.has_value() && decoded_header->size() >= 1) {
      uint8_t decoded_l_field = (*decoded_header)[0];
      if (decoded_l_field >= 10 && decoded_l_field <= 255) {
        this->wmbus_mode_ = WMBusMode::MODE_T;
        this->wmbus_block_ = WMBusBlock::BLOCK_A;
        this->length_field_ = decoded_l_field;
        this->expected_length_ = mode_t_packet_size(this->length_field_);
        this->rx_buffer_.insert(this->rx_buffer_.end(), header, header + 4);
        ESP_LOGD(TAG, "Mode T detected: L=0x%02X (decoded from 3-of-6), expected_length=%zu",
                 this->length_field_, this->expected_length_);
      } else {
        this->wmbus_mode_ = WMBusMode::MODE_C;
        this->wmbus_block_ = WMBusBlock::BLOCK_A;
        this->length_field_ = header[0];
        this->expected_length_ =
            mode_c_expected_length(this->length_field_, this->wmbus_block_, false);
        this->rx_buffer_.push_back(WMBUS_MODE_C_PREAMBLE);
        this->rx_buffer_.push_back(WMBUS_BLOCK_A_PREAMBLE);
        this->rx_buffer_.insert(this->rx_buffer_.end(), header, header + 4);
        ESP_LOGD(TAG, "Mode C (no preamble): L=0x%02X, expected_length=%zu",
                 this->length_field_, this->expected_length_);
      }
    } else {
      this->wmbus_mode_ = WMBusMode::MODE_C;
      this->wmbus_block_ = WMBusBlock::BLOCK_A;
      this->length_field_ = header[0];
      this->expected_length_ =
          mode_c_expected_length(this->length_field_, this->wmbus_block_, false);
      this->rx_buffer_.push_back(WMBUS_MODE_C_PREAMBLE);
      this->rx_buffer_.push_back(WMBUS_BLOCK_A_PREAMBLE);
      this->rx_buffer_.insert(this->rx_buffer_.end(), header, header + 4);
      ESP_LOGD(TAG, "Mode C (fallback): L=0x%02X, expected_length=%zu",
               this->length_field_, this->expected_length_);
    }
  }
  if (this->expected_length_ == 0) {
    ESP_LOGW(TAG, "Unable to determine expected frame length (block=%c, L=0x%02X)",
             static_cast<char>(this->wmbus_block_), this->length_field_);
    return false;
  }
  this->bytes_received_ = 4;
  if (this->expected_length_ < this->bytes_received_) {
    ESP_LOGW(TAG, "Expected length %zu smaller than bytes already read %zu, adjusting",
             this->expected_length_, this->bytes_received_);
    this->expected_length_ = this->bytes_received_;
  }
  ESP_LOGD(TAG, "Frame detected: mode=%c, block=%c, L=0x%02X, expected=%zu",
           static_cast<char>(this->wmbus_mode_),
           static_cast<char>(this->wmbus_block_), this->length_field_,
           this->expected_length_);
  if (this->expected_length_ < MAX_FIXED_LENGTH) {
    this->driver_->write_register(CC1101Register::PKTLEN,
                                   static_cast<uint8_t>(this->expected_length_));
    this->driver_->write_register(CC1101Register::PKTCTRL0, 0x00);
    this->length_mode_ = LengthMode::FIXED;
  }
  this->driver_->write_register(CC1101Register::FIFOTHR, RX_FIFO_THRESHOLD);
  bytes_in_fifo = this->driver_->read_status(CC1101Status::RXBYTES) & 0x7F;
  if (bytes_in_fifo > 0) {
    size_t bytes_remaining = this->expected_length_ - this->bytes_received_;
    size_t bytes_to_read = std::min(static_cast<size_t>(bytes_in_fifo), bytes_remaining);
    if (bytes_to_read > 0) {
      size_t old_size = this->rx_buffer_.size();
      this->rx_buffer_.resize(old_size + bytes_to_read);
      this->driver_->read_rx_fifo(this->rx_buffer_.data() + old_size, bytes_to_read);
      this->bytes_received_ += bytes_to_read;
    }
  }
  return true;
}
bool CC1101::read_data_() {
  bool gdo2 = (this->gdo2_pin_ != nullptr) ? this->gdo2_pin_->digital_read() : true;
  if (!gdo2 && this->bytes_received_ > 0) {
    uint8_t bytes_in_fifo = this->driver_->read_status(CC1101Status::RXBYTES) & 0x7F;
    if (bytes_in_fifo > 0) {
      ESP_LOGD(TAG, "GDO2 LOW detected, reading final %d bytes", bytes_in_fifo);
      size_t old_size = this->rx_buffer_.size();
      this->rx_buffer_.resize(old_size + bytes_in_fifo);
      this->driver_->read_rx_fifo(this->rx_buffer_.data() + old_size, bytes_in_fifo);
      this->bytes_received_ += bytes_in_fifo;
    }
    ESP_LOGD(TAG, "Frame complete via GDO2: %zu bytes", this->bytes_received_);
    return true;
  }
  if (this->check_rx_overflow_()) {
    ESP_LOGW(TAG, "RX FIFO overflow during read, aborting frame");
    this->rx_state_ = RxLoopState::INIT_RX;
    return false;
  }
  uint8_t bytes_in_fifo = this->driver_->read_status(CC1101Status::RXBYTES) & 0x7F;
  if (bytes_in_fifo > 0) {
    size_t bytes_remaining = this->expected_length_ - this->bytes_received_;
    size_t bytes_to_read;
    if (bytes_remaining <= bytes_in_fifo) {
      bytes_to_read = bytes_remaining;
    } else {
      bytes_to_read = (bytes_in_fifo > 1) ? (bytes_in_fifo - 1) : 0;
    }
    if (bytes_to_read > 0) {
      bytes_to_read = std::min(bytes_to_read, bytes_remaining);
      if (this->rx_buffer_.size() + bytes_to_read > MAX_FRAME_SIZE) {
        ESP_LOGW(TAG, "Frame too large");
        return false;
      }
      size_t old_size = this->rx_buffer_.size();
      this->rx_buffer_.resize(old_size + bytes_to_read);
      this->driver_->read_rx_fifo(this->rx_buffer_.data() + old_size,
                                   bytes_to_read);
      this->bytes_received_ += bytes_to_read;
    }
  }
  if (this->bytes_received_ >= this->expected_length_) {
    uint8_t bytes_in_fifo = this->driver_->read_status(CC1101Status::RXBYTES) & 0x7F;
    if (bytes_in_fifo > 0) {
      size_t old_size = this->rx_buffer_.size();
      this->rx_buffer_.resize(old_size + bytes_in_fifo);
      this->driver_->read_rx_fifo(this->rx_buffer_.data() + old_size,
                                   bytes_in_fifo);
    }
    return true;
  }
  return false;
}
void CC1101::set_idle_() {
  this->driver_->send_strobe(CC1101Strobe::SIDLE);
  uint8_t marc_state;
  for (int i = 0; i < 10; i++) {
    marc_state = this->driver_->read_status(CC1101Status::MARCSTATE);
    if (marc_state == static_cast<uint8_t>(CC1101State::IDLE))
      break;
    delay(1);
  }
}
bool CC1101::check_rx_overflow_() {
  uint8_t rxbytes = this->driver_->read_status(CC1101Status::RXBYTES);
  return (rxbytes & 0x80) != 0;
}
}
}
