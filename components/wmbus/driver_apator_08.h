/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_apator08.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Apator08: Driver
{
  Apator08() : Driver(std::string("apator08")) {};
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
    uint32_t usage = 0;
    // Seems that AT-WMBUS-08 is not decrypted. Maybe it is not true, but let's try ...
    size_t i = 11;
    usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
            ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
    ret_val = (usage / 3.0) / 1000.0;
    return ret_val;
  };
};