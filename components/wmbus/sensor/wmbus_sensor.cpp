#include "esphome/core/log.h"
#include "wmbus_sensor.h"

namespace esphome {
namespace wmbus {

static const char *const TAG = "wmbus_sensor";

WMBusSensor::WMBusSensor(const uint32_t id, const std::string type, const std::string key) {
  this->id = id;
  this->type = type;
  hex_to_bin(key, &(this->key));
}

void WMBusSensor::dump_config() {
  LOG_SENSOR("", "wM-Bus Sensor", this);
  std::string key = format_hex_pretty(this->key);
  key.erase(std::remove(key.begin(), key.end(), '.'), key.end());
  if (key.size()) {
    key.erase(key.size() - 5);
  }
  ESP_LOGCONFIG(TAG, "  Key: '%s'", key.c_str());
  ESP_LOGCONFIG(TAG, "  Type: %s", this->type.c_str());
  ESP_LOGCONFIG(TAG, "  ID: %zu [0x%08X]", this->id, this->id);
}

void WMBusSensor::publish_value(const float value) {
  this->publish_state(value);
}

int WMBusSensor::char_to_int(char input)
{
  if(input >= '0' && input <= '9') {
    return input - '0';
  }
  if(input >= 'A' && input <= 'F') {
    return input - 'A' + 10;
  }
  if(input >= 'a' && input <= 'f') {
    return input - 'a' + 10;
  }
  return -1;
}

bool WMBusSensor::hex_to_bin(const char* src, std::vector<unsigned char> *target)
{
  if (!src) return false;
  while(*src && src[1]) {
    if (*src == ' ' || *src == '#' || *src == '|' || *src == '_') {
      // Ignore space and hashes and pipes and underlines.
      src++;
    } 
    else {
      int hi = char_to_int(*src);
      int lo = char_to_int(src[1]);
      if (hi<0 || lo<0) return false;
      target->push_back(hi*16 + lo);
      src += 2;
    }
  }
  return true;
}

}  // namespace wmbus
}  // namespace esphome