#pragma once

#include "esphome/core/log.h"

#include "mbus.h"

#include "utils.h"

#include "decode3of6.h"
#include "m_bus_data.h"

#include <stdint.h>

#include "tmode_rf_settings.hpp"

#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include <string>


// CC1101 state machine
#define MARCSTATE_SLEEP            0x00
#define MARCSTATE_IDLE             0x01
#define MARCSTATE_XOFF             0x02
#define MARCSTATE_VCOON_MC         0x03
#define MARCSTATE_REGON_MC         0x04
#define MARCSTATE_MANCAL           0x05
#define MARCSTATE_VCOON            0x06
#define MARCSTATE_REGON            0x07
#define MARCSTATE_STARTCAL         0x08
#define MARCSTATE_BWBOOST          0x09
#define MARCSTATE_FS_LOCK          0x0A
#define MARCSTATE_IFADCON          0x0B
#define MARCSTATE_ENDCAL           0x0C
#define MARCSTATE_RX               0x0D
#define MARCSTATE_RX_END           0x0E
#define MARCSTATE_RX_RST           0x0F
#define MARCSTATE_TXRX_SWITCH      0x10
#define MARCSTATE_RXFIFO_OVERFLOW  0x11
#define MARCSTATE_FSTXON           0x12
#define MARCSTATE_TX               0x13
#define MARCSTATE_TX_END           0x14
#define MARCSTATE_RXTX_SWITCH      0x15
#define MARCSTATE_TXFIFO_UNDERFLOW 0x16

#define RX_FIFO_START_THRESHOLD    0
#define RX_FIFO_THRESHOLD          10  // 44 bytes in Rx FIFO

#define FIXED_PACKET_LENGTH        0x00
#define INFINITE_PACKET_LENGTH     0x02


enum RxLoopState : uint8_t {
  INIT_RX       = 0,
  WAIT_FOR_SYNC = 1,
  WAIT_FOR_DATA = 2,
  READ_DATA     = 3,
  DATA_END      = 4,
};


typedef struct RxLoopData {
  uint16_t bytesRx;
  uint8_t  lengthField;         // The L-field in the WMBUS packet
  uint16_t length;              // Total number of bytes to to be read from the RX FIFO
  uint16_t bytesLeft;           // Bytes left to to be read from the RX FIFO
  uint8_t *pByteIndex;          // Pointer to current position in the byte array
  bool complete;                // Packet received complete
  RxLoopState state;
} RxLoopData;



namespace esphome {
namespace wmbus {

static const char *TAG_LL = "cc1101";

class RxLoop {
  public:


//

  bool init(uint8_t mosi, uint8_t miso, uint8_t clk, uint8_t cs,
            uint8_t gdo0, uint8_t gdo2, float freq) {
    bool retVal = false;
    this->gdo0 = gdo0;
    this->gdo2 = gdo2;
    pinMode(this->gdo0, INPUT);
    pinMode(this->gdo2, INPUT);
    ELECHOUSE_cc1101.setSpiPin(clk, miso, mosi, cs);

    ELECHOUSE_cc1101.Init();

    for (uint8_t i = 0; i < TMODE_RF_SETTINGS_LEN; i++) {
      ELECHOUSE_cc1101.SpiWriteReg(TMODE_RF_SETTINGS_BYTES[i << 1],
                                  TMODE_RF_SETTINGS_BYTES[(i << 1) + 1]);
    }

    uint32_t freq_reg = uint32_t(freq * 65536 / 26);
    uint8_t freq2 = (freq_reg >> 16) & 0xFF;
    uint8_t freq1 = (freq_reg >> 8) & 0xFF;
    uint8_t freq0 = freq_reg & 0xFF;

    ESP_LOGD(TAG_LL, "Set CC1101 frequency to %3.3fMHz [%02X %02X %02X]",
          freq/1000000, freq2, freq1, freq0);
          // don't use setMHZ() -- seems to be broken
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ2, freq2);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ1, freq1);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ0, freq0);

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SCAL);

    byte cc1101Version = ELECHOUSE_cc1101.SpiReadStatus(CC1101_VERSION);

    if ((cc1101Version != 0) && (cc1101Version != 255)) {
      retVal = true;
      ESP_LOGD(TAG_LL, "wMBus-lib: CC1101 version '%d'", cc1101Version);
      ELECHOUSE_cc1101.SetRx();
      ESP_LOGD(TAG_LL, "wMBus-lib: CC1101 initialized");
      // memset(&RXinfo, 0, sizeof(RXinfo)); // ??? dlaczego cala struktore zerowalem?
      delay(4);
    }
    else {
      ESP_LOGE(TAG_LL, "wMBus-lib: CC1101 initialization FAILED!");
    }

    return retVal;
  }

  bool task(){
    switch (rxLoop.state) {
      case INIT_RX:
        start();
        return false;

      // RX active, waiting for SYNC
      case WAIT_FOR_SYNC:
        if (digitalRead(this->gdo2)) { // assert when SYNC detected
          rxLoop.state = WAIT_FOR_DATA;
          sync_time_ = millis();
        }
        break;

      // waiting for enough data in Rx FIFO buffer
      case WAIT_FOR_DATA:
        if (digitalRead(this->gdo0)) { // assert when Rx FIFO buffer threshold reached
          uint8_t preamble[2];
          // Read the 3 first bytes,
          ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, rxLoop.pByteIndex, 3);
          rxLoop.bytesRx += 3;
          const uint8_t *currentByte = rxLoop.pByteIndex;
          // Mode C
          if (*currentByte == 0x54) {
            currentByte++;
            data_in.mode = 'C';
            // Block A
            if (*currentByte == 0xCD) {
              currentByte++;
              rxLoop.lengthField = *currentByte;
              rxLoop.length = packetSize(rxLoop.lengthField);
              data_in.block = 'A';
            }
            // Block B
            else if (*currentByte == 0x3D) {
              currentByte++;
              rxLoop.lengthField = *currentByte;
              rxLoop.length = 1 + rxLoop.lengthField;
              data_in.block = 'B';
            }
            // Unknown type, reinit loop
            else {
              // LOGE("Unknown type 0x%02X", *currentByte);
              rxLoop.state = INIT_RX;
              return false;
              // czy tu dac return czy tez inaczej rozwiazac powrot do poczatku?
            }
            *(rxLoop.pByteIndex) = rxLoop.lengthField;
            rxLoop.pByteIndex  += 2;
            rxLoop.bytesRx     -= 2;
          }
          // Mode T Block A
          else if (decode3OutOf6(rxLoop.pByteIndex, preamble)) {
            rxLoop.lengthField = preamble[0];
            data_in.lengthField = rxLoop.lengthField;
            rxLoop.length  = byteSize(packetSize(rxLoop.lengthField));
            data_in.mode = 'T';
            data_in.block = 'A';
          }
          // Unknown mode, reinit loop
          else {
            // LOGE("Unknown mode 0x%02X", *currentByte);
            rxLoop.state = INIT_RX;
            return false;
          }

          // Set CC1101 into length mode
          ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTLEN, (uint8_t)(rxLoop.length));
          ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, FIXED_PACKET_LENGTH);

          rxLoop.pByteIndex += 3;
          rxLoop.bytesLeft   = rxLoop.length - 3;
          rxLoop.state = READ_DATA;
          max_wait_time_ += extra_time_;

          ELECHOUSE_cc1101.SpiWriteReg(CC1101_FIFOTHR, RX_FIFO_THRESHOLD);
        }
        break;

      // waiting for more data in Rx FIFO buffer
      case READ_DATA:
        if (digitalRead(this->gdo0)) { // assert when Rx FIFO buffer threshold reached
          // Do not empty the Rx FIFO (See the CC1101 SWRZ020E errata note)
          uint8_t bytesInFIFO = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES) & 0x7F;        
          ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, rxLoop.pByteIndex, bytesInFIFO - 1);

          rxLoop.bytesLeft  -= (bytesInFIFO - 1);
          rxLoop.pByteIndex += (bytesInFIFO - 1);
          rxLoop.bytesRx    += (bytesInFIFO - 1);
          max_wait_time_ += extra_time_;
        }
        break;
    }

    uint8_t overfl = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES) & 0x80;
    // END OF PAKET
    if ((!overfl) && (!digitalRead(gdo2)) && (rxLoop.state > WAIT_FOR_DATA)) {
      ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, rxLoop.pByteIndex, (uint8_t)rxLoop.bytesLeft);
      rxLoop.state = DATA_END;
      rxLoop.bytesRx += rxLoop.bytesLeft;
      data_in.length  = rxLoop.bytesRx;
      if (rxLoop.length != data_in.length) {
        ESP_LOGE(TAG_LL, "Length problem: req(%d) != rx(%d)", rxLoop.length, data_in.length);
      }
      ESP_LOGD(TAG_LL, "Have %d bytes from CC1101 Rx", rxLoop.bytesRx);
      if (mBusDecode(data_in, this->returnFrame)) {
        ESP_LOGD(TAG_LL, "Packet OK.");
        rxLoop.complete = true;
        this->returnFrame.mode  = data_in.mode;
        this->returnFrame.block = data_in.block;
        this->returnFrame.rssi  = (int8_t)ELECHOUSE_cc1101.getRssi();
        this->returnFrame.lqi   = (uint8_t)ELECHOUSE_cc1101.getLqi();
      }
      else {
        ESP_LOGD(TAG_LL, "Error during decoding.");
      }
      rxLoop.state = INIT_RX;
      return rxLoop.complete;
    }
    start(false);
    return rxLoop.complete;
  }

  WMbusFrame get_frame() {
    return this->returnFrame;
  }

  private:
    uint8_t start(bool force = true) {
      // waiting to long for next part of data?
      bool reinit_needed = ((millis() - sync_time_) > max_wait_time_) ? true: false;
      if (!force) {
        if (!reinit_needed) {
          // already in RX?
          if (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) == MARCSTATE_RX) {
            return 0;
          }
        }
      }
      // init RX here, each time we're idle
      rxLoop.state = INIT_RX;
      sync_time_ = millis();
      max_wait_time_ = extra_time_;

      ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
      while((ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) != MARCSTATE_IDLE));
      ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);  //flush TXfifo
      ELECHOUSE_cc1101.SpiStrobe(CC1101_SFRX);  //flush RXfifo

      // Initialize RX info variable
      rxLoop.lengthField = 0;              // Length Field in the wireless MBUS packet
      rxLoop.length      = 0;              // Total length of bytes to receive packet
      rxLoop.bytesLeft   = 0;              // Bytes left to to be read from the RX FIFO
      rxLoop.bytesRx     = 0;              // ile otrzymalismy bajtow
      rxLoop.pByteIndex  = data_in.data;   // Pointer to current position in the byte array
      rxLoop.complete    = false;          // Packet Received

      this->returnFrame.frame.clear();
      this->returnFrame.rssi  = 0;
      this->returnFrame.lqi   = 0;
      this->returnFrame.mode  = 'X';
      this->returnFrame.block = 'X';

      std::fill( std::begin( data_in.data ), std::end( data_in.data ), 0 );
      data_in.length      = 0;
      data_in.lengthField = 0;
      data_in.mode        = 'X';
      data_in.block       = 'X';

      // Set RX FIFO threshold to 4 bytes
      ELECHOUSE_cc1101.SpiWriteReg(CC1101_FIFOTHR, RX_FIFO_START_THRESHOLD);
      // Set infinite length 
      ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, INFINITE_PACKET_LENGTH);

      ELECHOUSE_cc1101.SpiStrobe(CC1101_SRX);
      while((ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) != MARCSTATE_RX));

      rxLoop.state = WAIT_FOR_SYNC;

      return 1; // this will indicate we just have re-started RX
    }

    uint8_t gdo0{0};
    uint8_t gdo2{0};

    m_bus_data_t data_in{0}; // Data from Physical layer decoded to bytes
    WMbusFrame returnFrame;

    RxLoopData rxLoop;

    uint32_t sync_time_{0};
    uint8_t extra_time_{50};
    uint8_t max_wait_time_ = extra_time_;
};


}
}