#pragma once
#include "esphome/components/sensor/sensor.h"

#include "../base_sensor.h"

namespace esphome
{
    namespace wmbus_meter
    {
        class Sensor : public sensor::Sensor, public BaseSensor
        {
        public:
            void handle_update();
            void dump_config() override;
        };
    }
}
