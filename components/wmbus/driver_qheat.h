/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_qheat.cc
  Copyright (C) 2021-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Qheat: Driver
{
  Qheat() : Driver(std::string("qheat")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_energy_consumption_kwh", this->get_0C0D(telegram));
    add_to_map(ret_val, "total_energy_consumption_kwh", this->get_0C05(telegram));
    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
};