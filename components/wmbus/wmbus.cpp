#include "wmbus.h"
#include "version.h"

namespace esphome {
namespace wmbus {

static const char *TAG = "wmbus";

WMBusComponent::WMBusComponent() {}
WMBusComponent::~WMBusComponent() {}

void WMBusComponent::setup() {
  this->high_freq_.start();
  if (this->led_pin_ != nullptr) {
    this->led_pin_->setup();
    this->led_pin_->digital_write(false);
    led_on_ = false;
  }
  memset(this->mb_packet_, 0, sizeof(this->mb_packet_));
  if (!rf_mbus_init(this->spi_conf_.mosi->get_pin(), this->spi_conf_.miso->get_pin(),
                    this->spi_conf_.clk->get_pin(), this->spi_conf_.cs->get_pin(),
                    this->spi_conf_.gdo0->get_pin(), this->spi_conf_.gdo2->get_pin())) {
    this->mark_failed();
    ESP_LOGE(TAG, "CC1101 initialization failed.");
    return;
  }

  this->add_driver(new Amiplus());
  this->add_driver(new Apator08());
  this->add_driver(new Apator162());
  this->add_driver(new ApatorEITN());
  this->add_driver(new Bmeters());
  this->add_driver(new Elf());
  this->add_driver(new Evo868());
  this->add_driver(new FhkvdataIII());
  this->add_driver(new Hydrocalm3());
  this->add_driver(new Itron());
  this->add_driver(new Izar());
  this->add_driver(new Mkradio3());
  this->add_driver(new Mkradio4());
  this->add_driver(new Qheat());
  this->add_driver(new Qwater());
  this->add_driver(new Ultrimis());
  this->add_driver(new Unismart());
  this->add_driver(new Vario451());
}

void WMBusComponent::loop() {
  this->led_handler();
  int8_t rssi_dbm{0};
  uint8_t lqi{0};
  if (rf_mbus_task(this->mb_packet_, rssi_dbm, lqi, this->spi_conf_.gdo0->get_pin(), this->spi_conf_.gdo2->get_pin())) {
    uint8_t len_without_crc = crcRemove(this->mb_packet_, packetSize(this->mb_packet_[0]));
    std::vector<unsigned char> frame(this->mb_packet_, this->mb_packet_ + len_without_crc);
    std::string telegram = format_hex_pretty(frame);
    telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());

    // ToDo: add check for manufactures
    uint32_t meter_id = ((uint32_t)frame[7] << 24) | ((uint32_t)frame[6] << 16) |
                        ((uint32_t)frame[5] << 8)  | ((uint32_t)frame[4]);

    if (this->wmbus_listeners_.count(meter_id) > 0) {
      // for debug
      WMBusListener *text_debug{nullptr};
      if (this->wmbus_listeners_.count(0xAFFFFFF5) > 0) {
        text_debug = this->wmbus_listeners_[0xAFFFFFF5];
      }
      //
      auto *sensor = this->wmbus_listeners_[meter_id];
      auto selected_driver = this->drivers_[sensor->type];
      ESP_LOGI(TAG, "Using driver '%s' for ID [0x%08X] RSSI: %d dBm LQI: %d T: %s", selected_driver->get_name().c_str(), meter_id, rssi_dbm, lqi, telegram.c_str());
      float value{0};
      if (sensor->key.size()) {
        if (this->decrypt_telegram(frame, sensor->key)) {
          std::string decrypted_telegram = format_hex_pretty(frame);
          decrypted_telegram.erase(std::remove(decrypted_telegram.begin(), decrypted_telegram.end(), '.'), decrypted_telegram.end());
          ESP_LOGD(TAG, "Decrypted T : %s", decrypted_telegram.c_str());
        }
        else {
          std::string decrypted_telegram = format_hex_pretty(frame);
          decrypted_telegram.erase(std::remove(decrypted_telegram.begin(), decrypted_telegram.end(), '.'), decrypted_telegram.end());
          std::string key = format_hex_pretty(sensor->key);
          key.erase(std::remove(key.begin(), key.end(), '.'), key.end());
          if (key.size()) {
            key.erase(key.size() - 5);
          }
          ESP_LOGE(TAG, "Something was not OK during decrypting telegram for ID [0x%08X] '%s' key: '%s'", meter_id, selected_driver->get_name().c_str(), key.c_str());
          ESP_LOGE(TAG, "T : %s", telegram.c_str());
          ESP_LOGE(TAG, "T': %s", decrypted_telegram.c_str());
        }
      }
      if (selected_driver->get_value(frame, value)) {
        sensor->publish_value(value);
        // for debug
        if (text_debug != nullptr) {
          if (text_debug->type == "all") {
            std::string prefix = "telegram for ";
            prefix += sensor->type;
            text_debug->publish_value(prefix);
            std::string telegramik;
            int split = 100;
            int start = 0;
            int part = 1;
            while (start < telegram.size()) {
              telegramik = std::to_string(part++) + "  | ";
              telegramik += telegram.substr(start, split);
              text_debug->publish_value(telegramik);
              start += split;
            }
            std::string decoded_telegramik = format_hex_pretty(frame);
            split = 75;
            start = 0;
            part = 1;
            while (start < decoded_telegramik.size()) {
              telegramik = std::to_string(part++) + "' | ";
              telegramik += decoded_telegramik.substr(start, split);
              text_debug->publish_value(telegramik);
              start += split;
              split = 99;
            }
          }
          else if ((value > 500000) && (sensor->type == "apator162")) {
            text_debug->publish_value("apator162 strange value");
            std::string telegramik;
            int split = 100;
            int start = 0;
            int part = 1;
            while (start < telegram.size()) {
              telegramik = std::to_string(part++) + "  | ";
              telegramik += telegram.substr(start, split);
              text_debug->publish_value(telegramik);
              start += split;
            }
            std::string decoded_telegramik = format_hex_pretty(frame);
            split = 75;
            start = 0;
            part = 1;
            while (start < decoded_telegramik.size()) {
              telegramik = std::to_string(part++) + "' | ";
              telegramik += decoded_telegramik.substr(start, split);
              text_debug->publish_value(telegramik);
              start += split;
              split = 99;
            }
          }
        }
        //
        this->led_blink();
      }
      else {
        std::string not_ok_telegram = format_hex_pretty(frame);
        not_ok_telegram.erase(std::remove(not_ok_telegram.begin(), not_ok_telegram.end(), '.'), not_ok_telegram.end());
        ESP_LOGE(TAG, "Can't get value from telegram for ID [0x%08X] '%s'", meter_id, selected_driver->get_name().c_str());
        ESP_LOGE(TAG, "T : %s", not_ok_telegram.c_str());
      }
    }
    else {
      ESP_LOGD(TAG, "Meter ID [0x%08X] RSSI: %d dBm LQI: %d not found in configuration T: %s", meter_id, rssi_dbm, lqi, telegram.c_str());
    }
    if (!(this->clients_.empty())) {
      this->led_blink();
    }
    for (auto & client : this->clients_) {
      switch (client.format) {
        case FORMAT_HEX:
          {
            switch (client.transport) {
              case TRANSPORT_TCP:
                {
                  if (this->tcp_client_.connect(client.ip.str().c_str(), client.port)) {
                    this->tcp_client_.write((const uint8_t *) this->mb_packet_, len_without_crc);
                    this->tcp_client_.stop();
                  }
                }
                break;
              case TRANSPORT_UDP:
                {
                  this->udp_client_.beginPacket(client.ip.str().c_str(), client.port);
                  this->udp_client_.write((const uint8_t *) this->mb_packet_, len_without_crc);
                  this->udp_client_.endPacket();
                }
                break;
              default:
                ESP_LOGE(TAG, "Unknown transport!");
                break;
            }
          }
          break;
        case FORMAT_RTLWMBUS:
          {
            time_t current_time = this->time_->now().timestamp;
            char telegram_time[24];
            strftime(telegram_time, sizeof(telegram_time), "%Y-%m-%d %H:%M:%S.00Z", gmtime(&current_time));
            switch (client.transport) {
              case TRANSPORT_TCP:
                {
                  if (this->tcp_client_.connect(client.ip.str().c_str(), client.port)) {
                    this->tcp_client_.printf("T1;1;1;%s;%d;;;0x", telegram_time, rssi_dbm);
                    for (int i = 0; i < len_without_crc; i++){
                      this->tcp_client_.printf("%02X", this->mb_packet_[i]);
                    }
                    this->tcp_client_.print("\n");
                    this->tcp_client_.stop();
                  }
                }
                break;
              case TRANSPORT_UDP:
                {
                  this->udp_client_.beginPacket(client.ip.str().c_str(), client.port);
                  this->udp_client_.printf("T1;1;1;%s;%d;;;0x", telegram_time, rssi_dbm);
                  for (int i = 0; i < len_without_crc; i++){
                    this->udp_client_.printf("%02X", this->mb_packet_[i]);
                  }
                  this->udp_client_.print("\n");
                  this->udp_client_.endPacket();
                }
                break;
              default:
                ESP_LOGE(TAG, "Unknown transport!");
                break;
            }
          }
          break;
        default:
          ESP_LOGE(TAG, "Unknown format!");
          break;
      }
    }
    memset(this->mb_packet_, 0, sizeof(this->mb_packet_));
  }
}

bool WMBusComponent::decrypt_telegram(std::vector<unsigned char> &telegram, std::vector<unsigned char> &key) {
  bool ret_val = false;
  std::vector<unsigned char>::iterator pos;
  pos = telegram.begin();
  unsigned char iv[16];
  int i=0;
  for (int j=0; j<8; ++j) {
    iv[i++] = telegram[2+j];
  }
  for (int j=0; j<8; ++j) {
    iv[i++] = telegram[11];
  }
  pos = telegram.begin() + 15;
  int num_encrypted_bytes = 0;
  int num_not_encrypted_at_end = 0;

  if (decrypt_TPL_AES_CBC_IV(telegram, pos, key, iv,
                            &num_encrypted_bytes, &num_not_encrypted_at_end)) {
    uint32_t decrypt_check = 0x2F2F;
    uint32_t dc = (((uint32_t)telegram[15] << 8) | ((uint32_t)telegram[16]));
    if ( dc == decrypt_check) {
      ret_val = true;
    }
  }
  return ret_val;
}

void WMBusComponent::register_wmbus_listener(WMBusListener *listener) {
  this->wmbus_listeners_[listener->id] = listener;
}

void WMBusComponent::add_driver(Driver *driver) {
  this->drivers_[driver->get_name()] = driver;
}

void WMBusComponent::led_blink() {
  if (this->led_pin_ != nullptr) {
    if (!led_on_) {
      this->led_on_millis_ = millis();
      this->led_pin_->digital_write(true);
      led_on_ = true;
    }
  }
}

void WMBusComponent::led_handler() {
  if (this->led_pin_ != nullptr) {
    if (led_on_) {
      if ((millis() - this->led_on_millis_) >= this->led_blink_time_) {
        this->led_pin_->digital_write(false);
        led_on_ = false;
      }
    }
  }
}

const LogString *WMBusComponent::format_to_string(Format format) {
  switch (format) {
    case FORMAT_HEX:
      return LOG_STR("hex");
    case FORMAT_RTLWMBUS:
      return LOG_STR("rtl-wmbus");
    default:
      return LOG_STR("unknown");
  }
}

const LogString *WMBusComponent::transport_to_string(Transport transport) {
  switch (transport) {
    case TRANSPORT_TCP:
      return LOG_STR("TCP");
    case TRANSPORT_UDP:
      return LOG_STR("UDP");
    default:
      return LOG_STR("unknown");
  }
}

void WMBusComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "wM-Bus v%s:", MY_VERSION);
  if (this->clients_.size() > 0) {
    ESP_LOGCONFIG(TAG, "  Clients:");
    for (auto & client : this->clients_) {
      ESP_LOGCONFIG(TAG, "    %s: %s:%d %s [%s]",
                    client.name.c_str(),
                    client.ip.str().c_str(),
                    client.port,
                    LOG_STR_ARG(transport_to_string(client.transport)),
                    LOG_STR_ARG(format_to_string(client.format)));
    }
  }
  if (this->led_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  LED:");
    LOG_PIN("    Pin: ", this->led_pin_);
    ESP_LOGCONFIG(TAG, "    Duration: %d ms", this->led_blink_time_);
  }
  ESP_LOGCONFIG(TAG, "  CC1101 SPI bus:");
  LOG_PIN("    MOSI Pin: ", this->spi_conf_.mosi);
  LOG_PIN("    MISO Pin: ", this->spi_conf_.miso);
  LOG_PIN("    CLK Pin:  ", this->spi_conf_.clk);
  LOG_PIN("    CS Pin:   ", this->spi_conf_.cs);
  LOG_PIN("    GDO0 Pin: ", this->spi_conf_.gdo0);
  LOG_PIN("    GDO2 Pin: ", this->spi_conf_.gdo2);
  if (this->drivers_.size() > 0) {
    std::string drivers = "  ";
    for (const auto& element : this->drivers_) {
      drivers += element.first + ", ";
    }
    drivers.erase(drivers.size() - 2);
    ESP_LOGCONFIG(TAG, "  Available drivers:%s", drivers.c_str());
  }
  else {
    ESP_LOGCONFIG(TAG, "  Check connection to CC1101!");
  }
}

}  // namespace wmbus
}  // namespace esphome
