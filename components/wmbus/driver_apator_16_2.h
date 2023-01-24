/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_apator162.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Apator162: Driver
{
  Apator162() : Driver(std::string("apator162")) {};
  bool get_value(std::vector<unsigned char> &telegram, float &water_usage) override {
    bool ret_val = false;
    uint32_t usage = 0;
    size_t i = 25;
    while (i < telegram.size()) {
      int c = telegram[i];
      int size = this->registerSize(c);
      if (c == 0xff) break; // An FF signals end of telegram padded to encryption boundary,
      // FFFFFFF623A where 4 last are perhaps crc or counter?
      i++;
      if (size == -1 || i+size >= telegram.size()) {
        break;
      }
      if (c == 0x10 && size == 4 && i+size < telegram.size()) {
        // We found the register representing the total
        usage = ((uint32_t)telegram[i+3] << 24) | ((uint32_t)telegram[i+2] << 16) |
                ((uint32_t)telegram[i+1] << 8)  | ((uint32_t)telegram[i+0]);
        water_usage = usage / 1000.0;
        ret_val = true;
        break;
      }
      i += size;
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
      // After 0x0F there is always:
      // next 4B : Date - In default frame
      // next 3B : Faults - In default frame 
          
      case 0x00: return 4; // Date 
      case 0x01: return 3; // Faults - In default frame f.ex. 0F 09 4D A1 97 18 02 00 -> 18 02 00 -> 00 02 18 -> 0x0218
      
      case 0xA1:
      case 0x10: return 4; // Total volume - In default frame
      
      case 0x11: return 2; // Flow

      case 0x40: return 6; // Detectors
      case 0x41: return 2; // Voltage
      case 0x42: return 4; // Energy
      case 0x43: return 2; // Life days - In default frame f.ex. 43 6E 0A -> 2670 days from first run

      case 0x71: return 9;
      case 0x73: return 1+4*4; // Historical data
      case 0x75: return 1+6*4; // Historical data
      case 0x7B: return 1+12*4; // Historical data

      case 0x80: 
      case 0x81: 
      case 0x82: 
      case 0x83: 
      case 0x84: 
      case 0x86: 
      case 0x87: return 10; // Events
      
      case 0x85:
      case 0x88: 
      case 0x8F: return 11; // Events

      case 0x8A: return 9; // Events
      
      case 0x8B:
      case 0x8C: return 6; // Events
      
      case 0x8E: return 7; // Events
      
      case 0xA0: return 4;
      
      case 0xA2: return 1;
      
      case 0xA3: return 7;
      
      case 0xA4: return 4;
      
      case 0xA5:
      case 0xA9:
      case 0xAF: return 1;
      
      case 0xA6: return 3;
      
      case 0xA7:
      case 0xA8:
      case 0xAA:
      case 0xAB: 
      case 0xAC: 
      case 0xAD: return 2;

      case 0xB0: return 5;
      case 0xB1: return 8;
      case 0xB2: return 16;
      case 0xB3: return 8;
      case 0xB4: return 2;
      case 0xB5: return 16;
      
      // Unknown
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