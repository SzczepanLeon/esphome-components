#include "sensor.h"

namespace esphome
{
    namespace wmbus_meter
    {
        static const char *TAG = "wmbus_meter.sensor";

        void Sensor::handle_update()
        {
            auto val = this->parent_->get_numeric_field(this->field_name);
            if (val.has_value())
                this->publish_state(*val);
        }
    }
}
