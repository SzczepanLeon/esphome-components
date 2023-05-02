#pragma once

#include "esphome/core/log.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/components/network/ip_address.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/sensor/sensor.h"

#include <map>
#include <string>

//wMBus lib
#include "rf_mbus.hpp"
#include "crc.hpp"
#include "mbus_packet.hpp"
#include "utils.hpp"

#include "drivers.h"

#include <WiFiClient.h>
#include <WiFiUdp.h>

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

struct Client {
  std::string name;
  network::IPAddress ip;
  uint16_t port;
  Transport transport;
  Format format;
};

class WMBusListener {
  public:
    WMBusListener(const uint32_t id, const std::string type, const std::string key);
    uint32_t id;
    std::string type;
    std::vector<unsigned char> key{};
    std::map<std::string, sensor::Sensor *> sensors_{};
    void add_sensor(std::string type, sensor::Sensor *sensor) {
      this->sensors_[type] = sensor;
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
                    InternalGPIOPin *gdo0, InternalGPIOPin *gdo2) {
      this->spi_conf_.mosi = mosi;
      this->spi_conf_.miso = miso;
      this->spi_conf_.clk  = clk;
      this->spi_conf_.cs   = cs;
      this->spi_conf_.gdo0 = gdo0;
      this->spi_conf_.gdo2 = gdo2;
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
    const LogString *format_to_string(Format format);
    const LogString *transport_to_string(Transport transport);
    void add_driver(Driver *driver);
    bool decrypt_telegram(std::vector<unsigned char> &telegram, std::vector<unsigned char> &key);
    void led_blink();
    void led_handler();
    HighFrequencyLoopRequester high_freq_;
    GPIOPin *led_pin_{nullptr};
    Cc1101 spi_conf_{};
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
    rf_mbus rf_mbus_;
};

}  // namespace wmbus
}  // namespace esphome