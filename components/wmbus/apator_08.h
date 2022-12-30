/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_apator08.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

#include "wmbus_utils.hpp"

struct Apator08: Driver
{
  Apator08() : Driver(std::string("apator08")) {};
  bool get_value(std::vector<unsigned char> &telegram, float &water_usage) override {
    bool ret_val = false;
    uint32_t usage = 0;
    std::vector<unsigned char> key(16,0);
    std::vector<unsigned char>::iterator pos;
    pos = telegram.begin();
    unsigned char iv[16];
    int i=0;
    for (int j=0; j<8; ++j) {
      iv[i++] = telegram[2+j];
    }
    for (int j=0; j<8; ++j) {
      iv[i++] = telegram[11];
    }
    pos = telegram.begin() + 15;
    int num_encrypted_bytes = 0;
    int num_not_encrypted_at_end = 0;

    if (decrypt_TPL_AES_CBC_IV(telegram, pos, key, iv,
                              &num_encrypted_bytes, &num_not_encrypted_at_end)) {
      size_t i = 11;
      usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
              ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
      water_usage = (usage / 3.0) / 1000.0;
      ret_val = true;
    }
    return ret_val;
  };

private:

};