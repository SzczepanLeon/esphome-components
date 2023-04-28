/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_elf.cc
  Copyright (C) 2021-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Elf: Driver
{
  Elf() : Driver(std::string("elf")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_energy_consumption_kwh", this->get_0E01(telegram));
    add_to_map(ret_val, "total_energy_consumption_kwh", this->get_0E0A(telegram));
    add_to_map(ret_val, "current_power_consumption_kw", this->get_0A2D(telegram));
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