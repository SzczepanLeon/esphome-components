#include "base_sensor.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace wmbus_meter
    {
        static const char *TAG = "wmbus_meter.base_sensor";

        void BaseSensor::set_field_name(std::string field_name)
        {
            this->field_name = field_name;
        }

        void BaseSensor::set_parent(Meter *parent)
        {
            Parented::set_parent(parent);
            this->parent_->on_telegram([this]()
                                       { this->handle_update(); });
        }

        void BaseSensor::dump_config()
        {
            ESP_LOGCONFIG(TAG, "wM-Bus Sensor:");
            ESP_LOGCONFIG(TAG, "  Field: %s", this->field_name.c_str());
            ESP_LOGCONFIG(TAG, "  Parent meter ID: %s", this->parent_->get_id().c_str());
        }
    }
}