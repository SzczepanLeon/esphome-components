#include "wmbus.h"
#include "esphome/core/log.h"

#include "version.h"

namespace esphome {
namespace wmbus {

static const char *TAG = "wmbus";

WMBusComponent::WMBusComponent() {}
WMBusComponent::~WMBusComponent() {}

void WMBusComponent::setup() {
  this->high_freq_.start();
  memset(this->mb_packet_, 0, sizeof(this->mb_packet_));
  rf_mbus_init(this->spi_conf_.mosi->get_pin(), this->spi_conf_.miso->get_pin(),
                this->spi_conf_.clk->get_pin(), this->spi_conf_.cs->get_pin(),
                this->spi_conf_.gdo0->get_pin(), this->spi_conf_.gdo2->get_pin());

  drivers_["izar"] = std::make_shared<Izar>();
  // drivers_["evo868"] = std::make_shared<Evo868>();
  drivers_["unismart"] = std::make_shared<Unismart>();
  drivers_["apator08"] = std::make_shared<Apator08>();
  drivers_["apator162"] = std::make_shared<Apator162>();
}

void WMBusComponent::loop() {
  int rssi{0};
  if (rf_mbus_task(this->mb_packet_, rssi, this->spi_conf_.gdo0->get_pin(), this->spi_conf_.gdo2->get_pin())) {

    uint8_t len_without_crc = crcRemove(this->mb_packet_, packetSize(this->mb_packet_[0]));
    std::vector<unsigned char> frame(this->mb_packet_, this->mb_packet_ + len_without_crc);
    std::string telegram = format_hex_pretty(frame);
    telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());

    uint32_t meter_id = ((uint32_t)frame[7] << 24) | ((uint32_t)frame[6] << 16) |
                        ((uint32_t)frame[5] << 8)  | ((uint32_t)frame[4]);

    if (this->wmbus_listeners_.count(meter_id) > 0) {
      auto *sensor = this->wmbus_listeners_[meter_id];
      auto selected_driver = this->drivers_[sensor->type];
      ESP_LOGI(TAG, "Using driver '%s' for ID [0x%08X] T: %s", selected_driver->get_name().c_str(), meter_id, telegram.c_str());
      float value{0};
      if (selected_driver->get_value(frame, value)) {
        sensor->publish_value(value);
      }
      else {
        std::string decoded_telegram = format_hex_pretty(frame);
        decoded_telegram.erase(std::remove(decoded_telegram.begin(), decoded_telegram.end(), '.'), decoded_telegram.end());
        ESP_LOGE(TAG, "Something was not OK during decoding telegram for ID [0x%08X] '%s'", meter_id, selected_driver->get_name().c_str());
        ESP_LOGE(TAG, "T : %s", telegram.c_str());
        ESP_LOGE(TAG, "T': %s", decoded_telegram.c_str());
      }
    }
    else {
      ESP_LOGD(TAG, "Meter ID [0x%08X] not found in configuration T: %s", meter_id, telegram.c_str());
    }
    memset(this->mb_packet_, 0, sizeof(this->mb_packet_));
  }
}

void WMBusComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "wM-Bus v%s:", MY_VERSION);
  ESP_LOGCONFIG(TAG, "  CC1101 SPI bus:");
  LOG_PIN("    MOSI Pin: ", this->spi_conf_.mosi);
  LOG_PIN("    MISO Pin: ", this->spi_conf_.miso);
  LOG_PIN("    CLK Pin:  ", this->spi_conf_.clk);
  LOG_PIN("    CS Pin:   ", this->spi_conf_.cs);
  LOG_PIN("    GDO0 Pin: ", this->spi_conf_.gdo0);
  LOG_PIN("    GDO2 Pin: ", this->spi_conf_.gdo2);
  std::string drivers = "  ";
  for (const auto& element : this->drivers_) {
    drivers += element.first + ", ";
  }
  drivers.erase(drivers.size() - 2);
  ESP_LOGCONFIG(TAG, "  Available drivers:%s", drivers.c_str());
}

void WMBusComponent::register_wmbus_listener(WMBusListener *listener) {
  this->wmbus_listeners_[listener->id] = listener;
}

}  // namespace wmbus
}  // namespace esphome