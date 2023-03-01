#include "esphome/core/log.h"
#include "wmbus_text_sensor.h"

namespace esphome {
namespace wmbus {

static const char *const TAG = "wmbus_text_sensor";

WMBusTextSensor::WMBusTextSensor(const uint32_t id, const std::string type) {
  this->id = id;
  this->type = type;
}

void WMBusTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "wM-Bus Text Sensor", this);
}

void WMBusTextSensor::publish_value(const std::string &value) {
  this->publish_state(value);
}

}  // namespace wmbus
}  // namespace esphome