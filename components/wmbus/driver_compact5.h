/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_compact5.cc
  Copyright (C) 2019-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Compact5: Driver
{
  Compact5() : Driver(std::string("compact5")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "current_heating_kwh", this->get_current_heating_kwh(telegram));
    add_to_map(ret_val, "previous_heating_kwh", this->get_previous_heating_kwh(telegram));

    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
  esphome::optional<float> get_current_heating_kwh(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    size_t i = 11;

    ret_val = (((uint32_t)telegram[i+8] << 8) + (uint32_t)telegram[i+7]);

    return ret_val;
  };

  esphome::optional<float> get_previous_heating_kwh(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    size_t i = 11;

    ret_val = (((uint32_t)telegram[i+4] << 8) + (uint32_t)telegram[i+3]);

    return ret_val;
  };
};