#include "text_sensor.h"

namespace esphome
{
    namespace wmbus_meter
    {
        static const char *TAG = "wmbus_meter.text_sensor";

        void TextSensor::handle_update()
        {
            auto val = this->parent_->get_string_field(this->field_name);
            if (val.has_value())
                this->publish_state(*val);
        }
    } // namespace wmbus_meter
} // namespace esphome
