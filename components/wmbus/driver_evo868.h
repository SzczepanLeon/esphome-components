/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_evo868.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Evo868: Driver
{
  Evo868() : Driver(std::string("evo868")) {};
  bool get_value(std::vector<unsigned char> &telegram, float &water_usage) override {
    bool ret_val = false;
    uint32_t usage = 0;
    
    // ToDo
    return ret_val;
  };

private:

};