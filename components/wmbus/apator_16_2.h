/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_apator162.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

#include "aes.h"
#include "wmbus_utils.h"

struct Apator162: Driver
{
  Apator162() : Driver(std::string("apator162")) {};
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
      size_t i = 25;
      while (i < telegram.size())
      {
        int c = telegram[i];
        int size = this->registerSize(c);
        if (c == 0xff) break; // An FF signals end of telegram padded to encryption boundary,
        // FFFFFFF623A where 4 last are perhaps crc or counter?
        i++;
        if (size == -1 || i+size >= telegram.size())
        {
          break;
        }
        if (c == 0x10 && size == 4 && i+size < telegram.size())
        {
          // We found the register representing the total
          usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
                  ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
          water_usage = usage / 1000.0;
          ret_val = true;
        }
        i += size;
      }
    }
    return ret_val;
  };

private:
  int registerSize(int c)
  {
    switch (c)
    {
        // case 0x0f: return 3; // Payload often starts with 0x0f,
        // which  also means dif = manufacturer data follows.
    case 0x10: return 4; // Total volume

    case 0x40: return 2;
    case 0x41: return 2;
    case 0x42: return 4;
    case 0x43: return 2;

    case 0x71: return 9;
    case 0x73: return 1+4*4; // Historical data
    case 0x75: return 1+6*4; // Historical data
    case 0x7B: return 1+12*4; // Historical data

    case 0x80: return 10;
    case 0x81: return 10;
    case 0x82: return 10;
    case 0x83: return 10;
    case 0x84: return 10;
    case 0x86: return 10;
    case 0x87: return 10;

    case 0xA0: return 4;

    case 0xB0: return 3;
    case 0xB1: return 3;
    case 0xB2: return 3;
    case 0xB3: return 3;
    case 0xB4: return 3;
    case 0xB5: return 3;
    case 0xB6: return 3;
    case 0xB7: return 3;
    case 0xB8: return 3;
    case 0xB9: return 3;
    case 0xBA: return 3;
    case 0xBB: return 3;
    case 0xBC: return 3;
    case 0xBD: return 3;
    case 0xBE: return 3;
    case 0xBF: return 3;

    case 0xC0: return 3;
    case 0xC1: return 3;
    case 0xC2: return 3;
    case 0xC3: return 3;
    case 0xC4: return 3;
    case 0xC5: return 3;
    case 0xC6: return 3;
    case 0xC7: return 3;

    case 0xD0: return 3;
    case 0xD3: return 3;

    case 0xF0: return 4;
    }
    return -1;
  }
};