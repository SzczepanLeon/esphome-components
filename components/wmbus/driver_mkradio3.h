/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_mkradio3.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Mkradio3: Driver
{
  Mkradio3() : Driver(std::string("mkradio3")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_water_m3", this->get_total_water_m3(telegram));

    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
  esphome::optional<float> get_total_water_m3(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    size_t i = 11;

    ret_val = ((((uint32_t)telegram[i+4] << 8) + (uint32_t)telegram[i+3]) / 10.0) + 
              ((((uint32_t)telegram[i+8] << 8) + (uint32_t)telegram[i+7]) / 10.0);

    return ret_val;
  };
};