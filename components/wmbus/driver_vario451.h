/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_vario451.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Vario451: Driver
{
  Vario451() : Driver(std::string("vario451")) {};
  bool get_value(std::vector<unsigned char> &telegram, float &total_usage) override {
    bool ret_val = true;
    size_t i = 11;

    total_usage = ((((uint32_t)telegram[i+4] << 8) + (uint32_t)telegram[i+3]) / 1000.0) + 
                  ((((uint32_t)telegram[i+8] << 8) + (uint32_t)telegram[i+7]) / 1000.0);
    // in kWh
    total_usage = total_usage * 277.777 ;

    return ret_val;
  };

private:

};