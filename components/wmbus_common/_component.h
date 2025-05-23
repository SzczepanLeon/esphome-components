#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace wmbus_common
    {
        static const char *TAG = "wmbus_common";
        class WMBusCommon : public Component
        {
        public:
            WMBusCommon(std::vector<std::string> drivers) : drivers_(drivers) {}
            void dump_config() override
            {
                ESP_LOGCONFIG(TAG, "wM-Bus Component:");
                ESP_LOGCONFIG(TAG, "  Loaded drivers:");
                for (const auto &driver : this->drivers_)
                    ESP_LOGCONFIG(TAG, "    - %s", driver.c_str());
            }

        protected:
            std::vector<std::string> drivers_;
        };
    } // namespace wmbus_common
} // namespace esphome