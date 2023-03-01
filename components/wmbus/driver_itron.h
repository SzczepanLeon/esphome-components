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
  bool get_value(std::vector<unsigned char> &telegram, float &water_usage) override {
    bool ret_val = false;
    uint32_t usage = 0;
    size_t i = 15;
    uint32_t total_register = 0x0413;
    while (i < telegram.size()) {
      uint32_t c = (((uint32_t)telegram[i+0] << 8) | ((uint32_t)telegram[i+1]));
      if (c == total_register) {
        i += 2;
        usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
                ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
        water_usage = usage / 1000.0;
        ret_val = true;
        break;
      }
      i++;
    }
    return ret_val;
  };

private:

};