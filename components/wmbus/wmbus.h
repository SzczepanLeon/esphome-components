#pragma once

#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#include <map>
#include <vector>
#include <string>

//wMBus lib
#include "rf_mbus.hpp"
#include "crc.hpp"
#include "mbus_packet.hpp"
#include "utils.hpp"

#include "drivers.h"

namespace esphome {
namespace wmbus {

class WMBusListener {
  public:
    uint32_t id;
    std::string type;
    virtual void publish_value(const float value) {};
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
    WMBusComponent();
    ~WMBusComponent();

    void setup() override;
    void loop() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::LATE; }
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

  private:

  protected:
    void publish_value_(const uint32_t id, const float val);
    HighFrequencyLoopRequester high_freq_;
    Cc1101 spi_conf_;
    uint8_t mb_packet_[291];
    std::vector<WMBusListener *> wmbus_listeners_{};
    std::map<std::string, std::shared_ptr<Driver>> drivers_;
};

}  // namespace wmbus
}  // namespace esphome