#ifdef USE_ESP32

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "preferences.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <soc/rtc.h>

#include <hal/cpu_hal.h>

#ifdef USE_ARDUINO
#include <Esp.h>
#else
#include <esp_clk_tree.h>

void setup();
void loop();
#endif

namespace esphome {

void IRAM_ATTR HOT yield() { vPortYield(); }
uint32_t IRAM_ATTR HOT millis() { return (uint32_t) (esp_timer_get_time() / 1000ULL); }
void IRAM_ATTR HOT delay(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }
uint32_t IRAM_ATTR HOT micros() { return (uint32_t) esp_timer_get_time(); }
void IRAM_ATTR HOT delayMicroseconds(uint32_t us) { delay_microseconds_safe(us); }
void arch_restart() {
  esp_restart();
  // restart() doesn't always end execution
  while (true) {  // NOLINT(clang-diagnostic-unreachable-code)
    yield();
  }
}

void arch_init() {
  // Enable the task watchdog only on the loop task (from which we're currently running)
#if defined(USE_ESP_IDF)
  esp_task_wdt_add(nullptr);
  // Idle task watchdog is disabled on ESP-IDF
#elif defined(USE_ARDUINO)
  enableLoopWDT();
  // Disable idle task watchdog on the core we're using (Arduino pins the task to a core)
#if defined(CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0) && CONFIG_ARDUINO_RUNNING_CORE == 0
  disableCore0WDT();
#endif
#if defined(CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1) && CONFIG_ARDUINO_RUNNING_CORE == 1
  disableCore1WDT();
#endif
#endif
}
void IRAM_ATTR HOT arch_feed_wdt() { esp_task_wdt_reset(); }

uint8_t progmem_read_byte(const uint8_t *addr) { return *addr; }
#if ESP_IDF_VERSION_MAJOR >= 5
uint32_t arch_get_cpu_cycle_count() { return esp_cpu_get_cycle_count(); }
#else
uint32_t arch_get_cpu_cycle_count() { return cpu_hal_get_cycle_count(); }
#endif
uint32_t arch_get_cpu_freq_hz() {
  uint32_t freq = 0;
#ifdef USE_ESP_IDF
  esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &freq);
#elif defined(USE_ARDUINO)
  freq = ESP.getCpuFreqMHz() * 1000000;
#endif
  return freq;
}

#ifdef USE_ESP_IDF
TaskHandle_t loop_task_handle = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void loop_task(void *pv_params) {
  setup();
  while (true) {
    loop();
  }
}

extern "C" void app_main() {
  esp32::setup_preferences();
  xTaskCreate(loop_task, "loopTask", 32768, nullptr, 1, &loop_task_handle);
}
#endif  // USE_ESP_IDF

#ifdef USE_ARDUINO
extern "C" void init() { esp32::setup_preferences(); }
#endif  // USE_ARDUINO

}  // namespace esphome

#endif  // USE_ESP32
