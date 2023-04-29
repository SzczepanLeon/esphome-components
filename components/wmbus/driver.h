#pragma once

#include <vector>
#include <string>
#include <map>
#include "esphome/core/optional.h"

struct Driver
{
  virtual ~Driver() = default; 

public:
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) = 0;

  std::string get_name() { return this->driver_type_; };

protected:
  Driver(std::string driver_type) : driver_type_(driver_type) {};

  void add_to_map(std::map<std::string, float> &values,
                  std::string name,
                  esphome::optional<float> value) {
    if (value.has_value()) {
      values[name] = *value;
    }
  };

  uint32_t bcd_2_int(const std::vector<unsigned char> &telegram, size_t start, size_t length) {
    uint32_t result{0};
    uint16_t l_half{0};
    uint16_t h_half{0};
    uint32_t factor{1};
    uint8_t i{0};
    while (i < length) {
      h_half = (telegram[start + i] & 0xF0) >> 4;
      l_half = telegram[start + i] & 0x0F;
      result += ((h_half * 10) + l_half) * factor;
      factor *= 100;
      i++;
    }
    return result;
  };

// /
  esphome::optional<float> get_0413(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0413;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
                ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
        ret_val = usage / 1000.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C0E(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C0E;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 4);
        // in kWh
        ret_val = usage / 3.6;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C0D(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C0D;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 4);
        // in kWh
        ret_val = usage / 36.00000;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C03(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C03;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 4);
        // in kWh
        ret_val = usage / 1000.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C05(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C05;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 4);
        // in kWh
        ret_val = usage / 10.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C06(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C06;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 4);
        // in kWh
        ret_val = usage / 1.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C13(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C13;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 4);
        ret_val = usage / 1000.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0E0A(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0E0A;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 6);
        // in kWh
        ret_val = usage / 36000.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0E01(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0E01;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 6);
        // in kWh
        ret_val = usage / 100000.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0A2D(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0A2D;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = bcd_2_int(telegram, i, 2);
        // in kW
        ret_val = usage / 10.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

  esphome::optional<float> get_0C943A(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    uint32_t usage = 0;
    size_t i = 11;
    uint32_t total_register = 0x0C943A;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 16) | ((uint32_t)telegram[i+1] << 8) | ((uint32_t)telegram[i+2]));
      if (c == total_register) {
        i += 3;
        usage = bcd_2_int(telegram, i, 4);
        ret_val = usage / 100.0;
        break;
      }
      i++;
    }
    return ret_val;
  };

private:
  Driver();
  std::string driver_type_;
};
