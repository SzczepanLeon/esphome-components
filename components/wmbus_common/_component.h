#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include <string>
#include <vector>

#include "_version.h"

// Declared in meters.h. Forward declared here instead of including that header
// so this file (pulled into main.cpp) does not inherit util.h's error/warning/
// debug macros.
struct DriverInfo;
DriverInfo *lookupDriver(std::string name);

namespace esphome {
namespace wmbus_common {
static const char *TAG = "wmbus_common";
class WMBusCommon : public Component {
public:
  WMBusCommon(std::vector<std::string> drivers) : drivers_(drivers) {}

  // The driver list below is what codegen *asked* for. Each driver registers
  // itself from a static initializer in its own translation unit, and nothing
  // else references those units - so a driver can be missing from the binary
  // (excluded at codegen, or dropped by the linker) while still being listed
  // here. Verify it for real instead of trusting the list.
  void setup() override {
    for (const auto &driver : this->drivers_) {
      if (lookupDriver(driver) == nullptr) {
        ESP_LOGE(TAG,
                 "Driver '%s' is configured but NOT present in the firmware - "
                 "meters using it will not work. Clean the build directory and "
                 "recompile.",
                 driver.c_str());
        this->missing_drivers_ = true;
      }
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "wM-Bus Component v%s-%s:", WMBUS_COMPONENT_VERSION,
                  WMBUSMETERS_VERSION);
    ESP_LOGCONFIG(TAG, "  Loaded drivers:");
    for (const auto &driver : this->drivers_) {
      if (lookupDriver(driver) == nullptr)
        ESP_LOGE(TAG, "   %s (MISSING)", driver.c_str());
      else
        ESP_LOGCONFIG(TAG, "   %s", driver.c_str());
    }
    if (this->missing_drivers_)
      ESP_LOGE(TAG, "  Some configured drivers are missing from this build!");
  }

protected:
  std::vector<std::string> drivers_;
  bool missing_drivers_{false};
};
} // namespace wmbus_common
} // namespace esphome