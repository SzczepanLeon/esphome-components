#pragma once

#include "esphome.h"
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/core/application.h"
#include "esphome/components/network/ip_address.h"
#include "esphome/components/time/real_time_clock.h"

#include <vector>
#include <string>
#include <algorithm>

#include <WiFiClient.h>
#include <WiFiUdp.h>

//wMBus lib
#include "rf_mbus.hpp"
#include "crc.hpp"
#include "mbus_packet.hpp"
#include "utils.hpp"

namespace esphome {
namespace wmbusgw {

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

struct Cc1101 {
  InternalGPIOPin *mosi{nullptr};
  InternalGPIOPin *miso{nullptr};
  InternalGPIOPin *clk{nullptr};
  InternalGPIOPin *cs{nullptr};
  InternalGPIOPin *gdo0{nullptr};
  InternalGPIOPin *gdo2{nullptr};
};

class WMBusGwComponent : public esphome::Component {
  public:
    WMBusGwComponent();
    ~WMBusGwComponent();

    void setup() override;
    void loop() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::LATE; }

    void set_reboot_timeout(uint32_t reboot_timeout) { this->reboot_timeout_ = reboot_timeout; }
    void set_time(time::RealTimeClock *time) { this->time_ = time; }
    void add_client(const std::string name,
                    const network::IPAddress ip,
                    const uint16_t port,
                    const Transport transport,
                    const Format format) {
      if (transport == TRANSPORT_TCP) {
        this->only_udp_ = false;
      }
      clients_.push_back({name, ip, port, transport, format});
    }

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

  private:
    const LogString *format_to_string(Format format);
    const LogString *transport_to_string(Transport transport);

  protected:
    HighFrequencyLoopRequester high_freq_;
    uint32_t reboot_timeout_;
    std::vector<Client> clients_;
    Cc1101 spi_conf_;
    WiFiClient tcp_client_;
    WiFiUDP udp_client_;
    uint8_t mb_packet_[291];
    time::RealTimeClock *time_;
    char telegram_time_[24];
    uint32_t last_connected_{0};
    bool only_udp_{true};
};

}  // namespace wmbusgw
}  // namespace esphome
