#include "sensor.h"

namespace esphome {
namespace wmbus_meter {
static const char *TAG = "wmbus_meter.sensor";

void Sensor::handle_update() {
  auto val = this->parent_->get_numeric_field(this->field_name);
  if (val.has_value())
    this->publish_state(*val);
}

void Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "wM-Bus Sensor:");
  ESP_LOGCONFIG(TAG, "  Parent meter ID: 0x%s",
                this->parent_->get_id().c_str());
  ESP_LOGCONFIG(TAG, "  Field: '%s'", this->field_name.c_str());
  LOG_SENSOR("  ", "Name:", this);
}
} // namespace wmbus_meter
} // namespace esphome
