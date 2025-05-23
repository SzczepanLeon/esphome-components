#pragma once
#include "esphome/core/automation.h"

#include "wmbus_meter.h"

namespace esphome
{
    namespace wmbus_meter
    {
        class TelegramTrigger : public Trigger<Meter &>
        {
        public:
            explicit TelegramTrigger(Meter *meter)
            {
                meter->on_telegram([this, meter]()
                                   { this->trigger(*meter); });
            }
        };

    }

}
