/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_fhkvdataiii.cc
  Copyright (C) 2019-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct FhkvdataIII: Driver
{
  FhkvdataIII() : Driver(std::string("fhkvdataiii")) {};
  bool get_value(std::vector<unsigned char> &telegram, float &current_hca) override {
    bool ret_val = true;
    size_t i = 11;

    current_hca = (((uint32_t)telegram[i+8] << 8) + (uint32_t)telegram[i+7]);

    return ret_val;
  };

private:

};