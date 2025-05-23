#pragma once
#include "transceiver.h"

namespace esphome
{
    namespace wmbus_radio
    {
        class SX1276 : public RadioTransceiver
        {
        public:
            void setup() override;
            optional<uint8_t> read() override;
            void restart_rx() override;
            int8_t get_rssi() override;
            const char * get_name() override;
        };
    }
}
