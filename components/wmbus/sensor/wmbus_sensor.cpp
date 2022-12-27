#include "esphome/core/log.h"
#include "wmbus_sensor.h"

namespace esphome {
namespace wmbus {

static const char *const TAG = "wmbus_sensor";

WMBusSensor::WMBusSensor(const uint32_t id, const std::string type) {
  this->id = id;
  this->type = type;
}

void WMBusSensor::dump_config() {
  LOG_SENSOR("", "wM-Bus Sensor", this);
  ESP_LOGCONFIG(TAG, "  Type: %s", this->type.c_str());
  ESP_LOGCONFIG(TAG, "  ID: %d [0x%08X]", this->id, this->id);
}

void WMBusSensor::publish_value(const float value) {
  this->publish_state(value);
}

}  // namespace wmbus
}  // namespace esphome