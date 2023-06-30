#pragma once

#include "esphome/components/wmbus/wmbus.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace wmbus {

class WMBusTextSensor : public WMBusListener, public text_sensor::TextSensor, public Component {
  public:
    WMBusTextSensor(const uint32_t id, const std::string type);
    void dump_config() override;
    void publish_value(const std::string &value) override;
};

}  // namespace wmbus
}  // namespace esphome