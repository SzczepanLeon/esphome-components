#include "component.h"

#include "freertos/queue.h"
#include "freertos/task.h"

#define ASSERT(expr, expected, before_exit)                                    \
  {                                                                            \
    auto result = (expr);                                                      \
    if (!!result != expected) {                                                \
      ESP_LOGE(TAG, "Assertion failed: %s -> %d", #expr, result);              \
      before_exit;                                                             \
      return;                                                                  \
    }                                                                          \
  }

#define ASSERT_SETUP(expr) ASSERT(expr, 1, this->mark_failed())

namespace esphome {
namespace wmbus_radio {
static const char *TAG = "wmbus";

void Radio::setup() {
  ASSERT_SETUP(this->packet_queue_ = xQueueCreate(3, sizeof(Packet *)));

  ASSERT_SETUP(xTaskCreate((TaskFunction_t)this->receiver_task, "radio_recv",
                           3 * 1024, this, 2, &(this->receiver_task_handle_)));

  ESP_LOGI(TAG, "Receiver task created [%p]", this->receiver_task_handle_);

  this->radio->attach_data_interrupt(Radio::wakeup_receiver_task_from_isr,
                                     &(this->receiver_task_handle_));
}

void Radio::loop() {
  Packet *p;
  if (xQueueReceive(this->packet_queue_, &p, 0) != pdPASS)
    return;

  auto frame = p->convert_to_frame();

  if (!frame)
    return;

  ESP_LOGI(TAG, "Have data from radio (%zu bytes) [RSSI: %ddBm, mode: %s]",
           frame->data().size(), frame->rssi(), toString(frame->link_mode()));

  uint8_t packet_handled = 0;
  for (auto &handler : this->handlers_)
    handler(&frame.value());

  if (frame->handlers_count())
    ESP_LOGI(TAG, "Telegram handled by %d handlers", frame->handlers_count());
  else {
    ESP_LOGW(TAG, "Telegram not handled by any handler");
    Telegram t;
    if (t.parseHeader(frame->data()) && t.addresses.empty()) {
      ESP_LOGW(TAG, "Check if telegram can be parsed on:");
    } else {
      ESP_LOGW(TAG, "Check if telegram with address %s can be parsed on:",
               t.addresses.back().id.c_str());
    }
    ESP_LOGW(TAG,
             (std::string{"https://wmbusmeters.org/analyze/"} + frame->as_hex())
                 .c_str());
  }
}

void Radio::wakeup_receiver_task_from_isr(TaskHandle_t *arg) {
  BaseType_t xHigherPriorityTaskWoken;
  vTaskNotifyGiveFromISR(*arg, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Radio::receive_frame() {
  this->radio->restart_rx();

  if (!ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(60000))) {
    ESP_LOGD(TAG, "Radio interrupt timeout");
    return;
  }
  auto packet = std::make_unique<Packet>();

  if (!this->radio->read_in_task(packet->rx_data_ptr(),
                                 packet->rx_capacity())) {
    ESP_LOGV(TAG, "Failed to read preamble");
    return;
  }

  if (!packet->calculate_payload_size()) {
    ESP_LOGD(TAG, "Cannot calculate payload size");
    return;
  }

  if (!this->radio->read_in_task(packet->rx_data_ptr(),
                                 packet->rx_capacity())) {
    ESP_LOGW(TAG, "Failed to read data");
    return;
  }

  packet->set_rssi(this->radio->get_rssi());
  auto packet_ptr = packet.get();

  if (xQueueSend(this->packet_queue_, &packet_ptr, 0) == pdTRUE) {
    ESP_LOGV(TAG, "Queue items: %zu",
             uxQueueMessagesWaiting(this->packet_queue_));
    ESP_LOGV(TAG, "Queue send success");
    packet.release();
  } else
    ESP_LOGW(TAG, "Queue send failed");
}

void Radio::receiver_task(Radio *arg) {
  int counter = 0;
  while (true)
    arg->receive_frame();
}

void Radio::add_frame_handler(std::function<void(Frame *)> &&callback) {
  this->handlers_.push_back(std::move(callback));
}

} // namespace wmbus_radio
} // namespace esphome
