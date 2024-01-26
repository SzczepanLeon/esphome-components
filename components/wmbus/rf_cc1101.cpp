#include "rf_cc1101.h"

namespace esphome {
namespace wmbus {

    static const char *TAG = "cc1101";

    void myTrace(void) {
        esphome::ESP_LOGI(TAG, "My trace info.");
    }

}
}