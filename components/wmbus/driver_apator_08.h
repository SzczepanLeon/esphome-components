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
  bool get_value(std::vector<unsigned char> &telegram, float &water_usage) override {
    bool ret_val = true;
    uint32_t usage = 0;
    // Seems that AT-WMBUS-08 is not decrypted. Maybe it is not true, but let's try ...
    size_t i = 11;
    usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
            ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
    water_usage = (usage / 3.0) / 1000.0;
    return ret_val;
  };

private:

};