#pragma once

#include <ELECHOUSE_CC1101_SRC_DRV.h>

const uint8_t TMODE_RF_SETTINGS_LEN = 47;

//based on https://www.ti.com/lit/an/swra234a/swra234a.pdf
const uint8_t TMODE_RF_SETTINGS_BYTES[] = {
    // Type B
    CC1101_IOCFG2,   0x06,  // 
    CC1101_IOCFG1,   0x2E,  // 
    CC1101_IOCFG0,   0x00,  // 
    CC1101_FIFOTHR,  0x07,  // 00
    CC1101_SYNC1,    0x54,  // 54
    CC1101_SYNC0,    0x3D,  // 3D
    CC1101_PKTLEN,   0xFF,  // 30
    CC1101_PKTCTRL1, 0x00,  // 00
    CC1101_PKTCTRL0, 0x00,  // 00
    CC1101_ADDR,     0x00,  // 00
    CC1101_CHANNR,   0x00,  // 00
    CC1101_FSCTRL1,  0x08,  // 08
    CC1101_FSCTRL0,  0x00,  // 00
    CC1101_FREQ2,    0x21,  // 21
    CC1101_FREQ1,    0x6B,  // 6B
    CC1101_FREQ0,    0xD0,  // D0
    CC1101_MDMCFG4,  0x5C,  // 5C
    CC1101_MDMCFG3,  0x04,  // 04
    CC1101_MDMCFG2,  0x06,  // 06
    CC1101_MDMCFG1,  0x22,  // 22
    CC1101_MDMCFG0,  0xF8,  // F8
    CC1101_DEVIATN,  0x44,  // 44
    CC1101_MCSM2,    0x07,
    CC1101_MCSM1,    0x00,  // 00
    CC1101_MCSM0,    0x18,  // 18
    CC1101_FOCCFG,   0x2E,  // 2E
    CC1101_BSCFG,    0xBF,  // BF
    CC1101_AGCCTRL2, 0x43,  // 43
    CC1101_AGCCTRL1, 0x09,  // 09
    CC1101_AGCCTRL0, 0xB5,  // B5
    CC1101_WOREVT1,  0x87,
    CC1101_WOREVT0,  0x6B,
    CC1101_WORCTRL,  0xFB,
    CC1101_FREND1,   0xB6,  // B6
    CC1101_FREND0,   0x10,  // 10
    CC1101_FSCAL3,   0xEA,  // EA
    CC1101_FSCAL2,   0x2A,  // 2A
    CC1101_FSCAL1,   0x00,  // 00
    CC1101_FSCAL0,   0x1F,  // 1F
    CC1101_RCCTRL1,  0x41,
    CC1101_RCCTRL0,  0x0,
    CC1101_FSTEST,   0x59,  // 59
    CC1101_PTEST,    0x7F,
    CC1101_AGCTEST,  0x3F,
    CC1101_TEST2,    0x81,  // 81
    CC1101_TEST1,    0x35,  // 35
    CC1101_TEST0,    0x09   // 09
};
