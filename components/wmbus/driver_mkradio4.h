/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_mkradio4.cc
  Copyright (C) 2019-2023 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Mkradio4: Driver
{
  Mkradio4() : Driver(std::string("mkradio4")) {};
  bool get_value(std::vector<unsigned char> &telegram, float &water_usage) override {
    bool ret_val = true;
    size_t i = 11;

    water_usage = ((((uint32_t)telegram[i+4] << 8) + (uint32_t)telegram[i+3]) / 10.0) + 
                  ((((uint32_t)telegram[i+8] << 8) + (uint32_t)telegram[i+7]) / 10.0);

    return ret_val;
  };

private:

};