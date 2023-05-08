/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_c5isf.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct C5isf: Driver
{
  C5isf() : Driver(std::string("c5isf")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_heating_kwh", this->get_0C0E(telegram));
    add_to_map(ret_val, "total_heating_kwh", this->get_0C03(telegram));

    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
};