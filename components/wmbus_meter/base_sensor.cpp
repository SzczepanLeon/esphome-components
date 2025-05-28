#include "base_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace wmbus_meter {
static const char *TAG = "wmbus_meter.base_sensor";

void BaseSensor::set_field_name(std::string field_name) {
  this->field_name = field_name;
}

void BaseSensor::set_parent(Meter *parent) {
  Parented::set_parent(parent);
  this->parent_->on_telegram([this]() { this->handle_update(); });
}
} // namespace wmbus_meter
} // namespace esphome