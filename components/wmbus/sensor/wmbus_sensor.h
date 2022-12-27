#pragma once

#include "esphome/components/wmbus/wmbus.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace wmbus {

class WMBusSensor : public WMBusListener, public sensor::Sensor, public Component {
  public:
    WMBusSensor(const uint32_t id, const std::string type);
    void dump_config() override;
    void publish_value(const float value) override;
};

}  // namespace wmbus
}  // namespace esphome