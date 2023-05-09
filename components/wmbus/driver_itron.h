/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_itron.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Itron: Driver
{
  Itron() : Driver(std::string("itron")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_water_m3", this->get_0C13(telegram));
    add_to_map(ret_val, "total_water_m3", this->get_0413(telegram));


    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
};