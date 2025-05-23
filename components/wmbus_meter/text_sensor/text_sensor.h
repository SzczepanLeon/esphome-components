#pragma once
#include "esphome/components/text_sensor/text_sensor.h"

#include "../base_sensor.h"

namespace esphome
{
    namespace wmbus_meter
    {
        class TextSensor : public text_sensor::TextSensor,
                           public BaseSensor
        {
        public:
            void handle_update() override;
        };
    }
}
