#pragma once

#include "esphome/core/log.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/components/network/ip_address.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <map>
#include <string>

#include "drivers.h"

#include <WiFiClient.h>
#include <WiFiUdp.h>

#include "rf_cc1101.h"
#include "m_bus_data.h"
#include "utils.h"
#include "crc.h"

#include <DriversWmbusMeters/utils.h>

namespace esphome {
namespace wmbus {

  enum Format : uint8_t {
    FORMAT_HEX      = 0,
    FORMAT_RTLWMBUS = 1,
  };

  enum Transport : uint8_t {
    TRANSPORT_TCP = 0,
    TRANSPORT_UDP = 1,
  };

  enum FrameMode : uint8_t {
    MODE_T1   = 0,
    MODE_C1   = 1,
    MODE_T1C1 = 2,
  };

  struct Client {
    std::string name;
    network::IPAddress ip;
    uint16_t port;
    Transport transport;
    Format format;
  };

  class WMBusListener {
    public:
      WMBusListener(const uint32_t id, const std::string type, const std::string key, const FrameMode framemode);
      WMBusListener(const uint32_t id, const std::string type, const std::string key);
      uint32_t id;
      std::string type;
      FrameMode framemode{};
      std::vector<unsigned char> key{};
      std::map<std::string, sensor::Sensor *> sensors_{};
      void add_sensor(std::string type, sensor::Sensor *sensor) {
        this->sensors_[type] = sensor;
      };
      text_sensor::TextSensor* text_sensor_{nullptr};
      void add_sensor(text_sensor::TextSensor *text_sensor) {
        this->text_sensor_ = text_sensor;
      };

      void dump_config();
      int char_to_int(char input);
      bool hex_to_bin(const std::string &src, std::vector<unsigned char> *target) { return hex_to_bin(src.c_str(), target); };
      bool hex_to_bin(const char* src, std::vector<unsigned char> *target);
  };

  struct Cc1101 {
    InternalGPIOPin *mosi{nullptr};
    InternalGPIOPin *miso{nullptr};
    InternalGPIOPin *clk{nullptr};
    InternalGPIOPin *cs{nullptr};
    InternalGPIOPin *gdo0{nullptr};
    InternalGPIOPin *gdo2{nullptr};
  };

  class WMBusComponent : public Component {
    public:
      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const override { return setup_priority::LATE; }
      void set_led_pin(GPIOPin *led) { this->led_pin_ = led; }
      void set_led_blink_time(uint32_t led_blink_time) { this->led_blink_time_ = led_blink_time; }
      void register_wmbus_listener(WMBusListener *listener);
      void add_cc1101(InternalGPIOPin *mosi, InternalGPIOPin *miso,
                      InternalGPIOPin *clk, InternalGPIOPin *cs,
                      InternalGPIOPin *gdo0, InternalGPIOPin *gdo2,
                      float frequency, bool sync_mode) {
        this->spi_conf_.mosi = mosi;
        this->spi_conf_.miso = miso;
        this->spi_conf_.clk  = clk;
        this->spi_conf_.cs   = cs;
        this->spi_conf_.gdo0 = gdo0;
        this->spi_conf_.gdo2 = gdo2;
        this->frequency_ = frequency;
        this->sync_mode_ = sync_mode;
      }
      void set_time(time::RealTimeClock *time) { this->time_ = time; }
      void set_log_unknown() { this->log_unknown_ = true; }
      void add_client(const std::string name,
                      const network::IPAddress ip,
                      const uint16_t port,
                      const Transport transport,
                      const Format format) {
        clients_.push_back({name, ip, port, transport, format});
      }

    private:

    protected:
      const LogString *framemode_to_string(FrameMode framemode);
      const LogString *format_to_string(Format format);
      const LogString *transport_to_string(Transport transport);
      void add_driver(Driver *driver);
      bool decrypt_telegram(std::vector<unsigned char> &telegram, std::vector<unsigned char> &key);
      void led_blink();
      void led_handler();
      HighFrequencyLoopRequester high_freq_;
      GPIOPin *led_pin_{nullptr};
      Cc1101 spi_conf_{};
      float frequency_{};
      bool sync_mode_{false};
      std::map<uint32_t, WMBusListener *> wmbus_listeners_{};
      std::map<std::string, Driver *> drivers_{};
      std::vector<Client> clients_{};
      WiFiClient tcp_client_;
      WiFiUDP udp_client_;
      time::RealTimeClock *time_{nullptr};
      uint32_t led_blink_time_{0};
      uint32_t led_on_millis_{0};
      bool led_on_{false};
      bool log_unknown_{false};
      RxLoop rf_mbus_;
  };

}  // namespace wmbus
}  // namespace esphome