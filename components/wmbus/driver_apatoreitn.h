/*
  Based on: https://github.com/weetmuts/wmbusmeters/blob/master/src/driver_apatoreitn.cc
  Copyright (C) 2019-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct ApatorEITN: Driver
{
  ApatorEITN() : Driver(std::string("apatoreitn")) {};
  virtual esphome::optional<std::map<std::string, float>> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "current_hca", this->get_current_hca(telegram));
    add_to_map(ret_val, "previous_hca", this->get_previous_hca(telegram));
    add_to_map(ret_val, "temp_room_avg_c", this->get_temp_room_avg_c(telegram));

    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
  esphome::optional<float> get_current_hca(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    size_t i = 10;
    if (telegram[i] == 0xB6) {
      i += telegram[i+1] + 2;
    }

    ret_val = (((uint32_t)telegram[i+9] << 8) + (uint32_t)telegram[i+8]);

    return ret_val;
  };

  esphome::optional<float> get_previous_hca(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    size_t i = 10;
    if (telegram[i] == 0xB6) {
      i += telegram[i+1] + 2;
    }

    ret_val = (((uint32_t)telegram[i+5] << 8) + (uint32_t)telegram[i+4]);

    return ret_val;
  };

  esphome::optional<float> get_temp_room_avg_c(std::vector<unsigned char> &telegram) {
    esphome::optional<float> ret_val{};
    size_t i = 10;
    if (telegram[i] == 0xB6) {
      i += telegram[i+1] + 2;
    }

    ret_val = (((uint32_t)telegram[i+15]) + ((uint32_t)telegram[i+14])/256.0);

    return ret_val;
  };
};