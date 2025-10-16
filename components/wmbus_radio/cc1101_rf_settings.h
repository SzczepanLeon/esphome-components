#pragma once
#include "cc1101_driver.h"
#include <array>
#include <utility>

namespace esphome {
namespace wmbus_radio {

constexpr std::array<std::pair<CC1101Register, uint8_t>, 47>
    CC1101_WMBUS_RF_SETTINGS = {{
        {CC1101Register::IOCFG2, 0x06},
        {CC1101Register::IOCFG1, 0x2E},
        {CC1101Register::IOCFG0, 0x00},
        {CC1101Register::FIFOTHR, 0x0A},
        {CC1101Register::SYNC1, 0x54},
        {CC1101Register::SYNC0, 0x3D},
        {CC1101Register::PKTLEN, 0xFF},
        {CC1101Register::PKTCTRL1, 0x00},
        {CC1101Register::PKTCTRL0, 0x00},
        {CC1101Register::ADDR, 0x00},
        {CC1101Register::CHANNR, 0x00},
        {CC1101Register::FSCTRL1, 0x08},
        {CC1101Register::FSCTRL0, 0x00},
        {CC1101Register::FREQ2, 0x21},
        {CC1101Register::FREQ1, 0x6B},
        {CC1101Register::FREQ0, 0xD0},
        {CC1101Register::MDMCFG4, 0x5C},
        {CC1101Register::MDMCFG3, 0x04},
        {CC1101Register::MDMCFG2, 0x06},
        {CC1101Register::MDMCFG1, 0x22},
        {CC1101Register::MDMCFG0, 0xF8},
        {CC1101Register::DEVIATN, 0x44},
        {CC1101Register::MCSM2, 0x07},
        {CC1101Register::MCSM1, 0x00},
        {CC1101Register::MCSM0, 0x18},
        {CC1101Register::FOCCFG, 0x2E},
        {CC1101Register::BSCFG, 0xBF},
        {CC1101Register::AGCCTRL2, 0x43},
        {CC1101Register::AGCCTRL1, 0x09},
        {CC1101Register::AGCCTRL0, 0xB5},
        {CC1101Register::WOREVT1, 0x87},
        {CC1101Register::WOREVT0, 0x6B},
        {CC1101Register::WORCTRL, 0xFB},
        {CC1101Register::FREND1, 0xB6},
        {CC1101Register::FREND0, 0x10},
        {CC1101Register::FSCAL3, 0xEA},
        {CC1101Register::FSCAL2, 0x2A},
        {CC1101Register::FSCAL1, 0x00},
        {CC1101Register::FSCAL0, 0x1F},
        {CC1101Register::RCCTRL1, 0x41},
        {CC1101Register::RCCTRL0, 0x00},
        {CC1101Register::FSTEST, 0x59},
        {CC1101Register::PTEST, 0x7F},
        {CC1101Register::AGCTEST, 0x3F},
        {CC1101Register::TEST2, 0x81},
        {CC1101Register::TEST1, 0x35},
        {CC1101Register::TEST0, 0x09},
    }};

inline void apply_wmbus_rf_settings(CC1101Driver &driver) {
  for (const auto &[reg, value] : CC1101_WMBUS_RF_SETTINGS)
    driver.write_register(reg, value);
}

inline void set_carrier_frequency(CC1101Driver &driver, float freq_mhz) {
  uint32_t freq_reg = static_cast<uint32_t>(freq_mhz * 65536.0f / 26.0f);
  uint8_t freq2 = (freq_reg >> 16) & 0xFF;
  uint8_t freq1 = (freq_reg >> 8) & 0xFF;
  uint8_t freq0 = freq_reg & 0xFF;
  driver.write_register(CC1101Register::FREQ2, freq2);
  driver.write_register(CC1101Register::FREQ1, freq1);
  driver.write_register(CC1101Register::FREQ0, freq0);
}

}
}
