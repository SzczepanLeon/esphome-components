#pragma once

#include "esphome/components/wmbus/wmbus.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace wmbus {

class WMBusSensor : public WMBusListener, public sensor::Sensor, public Component {
  public:
    WMBusSensor(const uint32_t id, const std::string type, const std::string key);
    void dump_config() override;
    void publish_value(const float value) override;

    int char_to_int(char input);
    bool hex_to_bin(const std::string &src, std::vector<unsigned char> *target) { return hex_to_bin(src.c_str(), target); };
    bool hex_to_bin(const char* src, std::vector<unsigned char> *target);
};

}  // namespace wmbus
}  // namespace esphome