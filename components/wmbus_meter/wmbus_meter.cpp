#include "wmbus_meter.h"

namespace esphome {
namespace wmbus_meter {
static const char *TAG = "wmbus_meter";

void Meter::set_meter_params(std::string id, std::string driver,
                             std::string key,
                             std::initializer_list<LinkMode> linkModes) {
  this->meter_id_ = id;
  this->driver_name_ = driver;

  for (auto linkMode : linkModes)
    this->link_modes_.addLinkMode(linkMode);

  MeterInfo meter_info;
  if (!meter_info.parse(driver + '-' + id, driver, id + ",", key)) {
    ESP_LOGE(TAG, "Cannot parse meter parameters (driver '%s', id '%s')",
             driver.c_str(), id.c_str());
    return;
  }

  this->meter = createMeter(&meter_info);
}

void Meter::setup() {
  // An unknown driver should be rejected while validating the configuration, so we just make sure here
  if (this->meter == nullptr) {
    ESP_LOGE(TAG, "Meter 0x%s was not created - no driver '%s'",
             this->meter_id_.c_str(), this->driver_name_.c_str());
    this->mark_failed();
  }
}
void Meter::set_radio(wmbus_radio::Radio *radio) {
  this->radio = radio;
  radio->add_frame_handler(
      [this](wmbus_radio::Frame *frame) { return this->handle_frame(frame); });
}
void Meter::dump_config() {
  // Failed components still get dumped, so this has to survive a null meter.
  ESP_LOGCONFIG(TAG, "wM-Bus Meter:");

  if (this->meter == nullptr) {
    ESP_LOGE(TAG, "  ID: 0x%s", this->meter_id_.c_str());
    ESP_LOGE(TAG, "  Driver: %s (NOT AVAILABLE)", this->driver_name_.c_str());
    return;
  }

  std::string id = this->get_id();
  std::string driver = this->get_driver();
  std::string key = this->get_key();

  ESP_LOGCONFIG(TAG, "  ID: 0x%s", id.c_str());
  ESP_LOGCONFIG(TAG, "  Driver: %s", driver.c_str());
  ESP_LOGCONFIG(TAG, "  Key: %s", key.c_str());
}

std::string Meter::get_id() {
  if (this->meter == nullptr)
    return this->meter_id_;

  std::vector<AddressExpression> address_expressions =
      this->meter->addressExpressions();
  return address_expressions.size() > 0 ? address_expressions[0].id : "unknown";
}

std::string Meter::get_driver() {
  if (this->meter == nullptr)
    return this->driver_name_;

  return this->meter->driverName().str();
}

std::string Meter::get_key() {
  if (this->meter == nullptr)
    return "unknown";

  MeterKeys *keys = this->meter->meterKeys();
  return keys->hasConfidentialityKey() ? bin2hex(keys->confidentiality_key)
                                       : "not-encrypted";
}

void Meter::handle_frame(wmbus_radio::Frame *frame) {
  // Runs from the radio callback, not loop(), so mark_failed() does not stop it.
  if (this->meter == nullptr)
    return;

  if (!this->link_modes_.has(frame->link_mode())) {
    ESP_LOGW(TAG, "Frame link mode %s not supported by meter %s",
             toString(frame->link_mode()), this->meter->name().c_str());
    return;
  }

  auto about =
      AboutTelegram(App.get_friendly_name(), frame->rssi(), FrameType::WMBUS);

  std::vector<Address> adresses;
  bool id_match = false;
  auto telegram = std::make_unique<Telegram>();

  this->meter->handleTelegram(about, frame->data(), false, &adresses, &id_match,
                              telegram.get());

  if (id_match) {
    this->last_telegram = std::move(telegram);
    this->defer([this]() {
      this->on_telegram_callback_manager();
      this->last_telegram = nullptr;
    });

    frame->mark_as_handled();
  }
}

std::string Meter::as_json(bool pretty_print) {
  std::string json;
  if (this->meter == nullptr)
    return json;

  this->meter->printMeter(this->last_telegram.get(), nullptr, nullptr, '\t',
                          &json, nullptr, nullptr, nullptr, pretty_print);
  return json;
}

optional<std::string> Meter::get_string_field(std::string field_name) {
  if (this->meter == nullptr)
    return {};

  if (field_name == "timestamp")
    return this->meter->datetimeOfUpdateHumanReadable();

  if (field_name == "timestamp_zulu")
    return this->meter->datetimeOfUpdateRobot();

  auto field_info = this->meter->findFieldInfo(field_name, Quantity::Text);
  if (field_info)
    return this->meter->getStringValue(field_info);

  return {};
}

optional<float> Meter::get_numeric_field(std::string field_name) {
  if (this->meter == nullptr)
    return {};

  // RSSI is not handled by meter but by telegram :/
  if (field_name == "rssi_dbm") {
    if (this->last_telegram == nullptr)
      return {};
    return this->last_telegram->about.rssi_dbm;
  }

  if (field_name == "timestamp")
    return this->meter->timestampLastUpdate();

  std::string name;
  Unit unit;
  extractUnit(field_name, &name, &unit);

  auto value = this->meter->getNumericValue(name, unit);

  if (!std::isnan(value))
    return value;

  return {};
}

void Meter::on_telegram(std::function<void()> &&callback) {
  this->on_telegram_callback_manager.add(std::move(callback));
}

} // namespace wmbus_meter
} // namespace esphome
