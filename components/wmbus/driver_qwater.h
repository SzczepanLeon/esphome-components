/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_qwater.cc
  Copyright (C) 2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Qwater: Driver
{
  Qwater() : Driver(std::string("qwater")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_water_m3", this->get_0C13(telegram));

    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
};