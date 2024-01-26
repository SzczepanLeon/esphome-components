#pragma once

#include "esphome/core/log.h"
#include "crc.h"

#include <stdint.h>

#include "tmode_rf_settings.hpp"

#include <ELECHOUSE_CC1101_SRC_DRV.h>

#include <string>
#include <vector>

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


#define BLOCK1A_SIZE 12     // Size of Block 1, format A
#define BLOCK1B_SIZE 10     // Size of Block 1, format B
#define BLOCK2B_SIZE 118    // Maximum size of Block 2, format B

// Helper macros, collides with MSVC's stdlib.h unless NOMINMAX is used
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#include "esphome/core/helpers.h"


enum RxLoopState : uint8_t {
  INIT_RX       = 0,
  WAIT_FOR_SYNC = 1,
  WAIT_FOR_DATA = 2,
  READ_DATA     = 3,
  DATA_END      = 4,
};

typedef struct {
    uint16_t  length;
    uint8_t   lengthField;
    uint8_t   data[500];
    char      mode;
    char      block;
} m_bus_data_t;


typedef struct RxLoopData {
  uint16_t bytesRx;
  uint8_t  lengthField;         // The L-field in the WMBUS packet
  uint16_t length;              // Total number of bytes to to be read from the RX FIFO
  uint16_t bytesLeft;           // Bytes left to to be read from the RX FIFO
  uint8_t *pByteIndex;          // Pointer to current position in the byte array
  bool complete;                // Packet received complete
  RxLoopState state;
} RxLoopData;

typedef struct WMbusFrame {
  std::vector<unsigned char> frame{};
  char      mode;
  char      block;
  int8_t rssi;
  uint8_t lqi;
} WMbusFrame;

namespace esphome {
namespace wmbus {

static const char *TAG_LL = "cc1101";

class rf_cc1101 {
  public:


  // Mapping from 6 bits to 4 bits. "3of6" coding used for Mode T
  static uint8_t decode3of6(uint8_t t_byte) {
    uint8_t retVal{0xFF}; // Error
    switch(t_byte) {
      case 22:  retVal = 0x0;  break;  // 0x16
      case 13:  retVal = 0x1;  break;  // 0x0D
      case 14:  retVal = 0x2;  break;  // 0x0E
      case 11:  retVal = 0x3;  break;  // 0x0B
      case 28:  retVal = 0x4;  break;  // 0x1C
      case 25:  retVal = 0x5;  break;  // 0x19
      case 26:  retVal = 0x6;  break;  // 0x1A
      case 19:  retVal = 0x7;  break;  // 0x13
      case 44:  retVal = 0x8;  break;  // 0x2C
      case 37:  retVal = 0x9;  break;  // 0x25
      case 38:  retVal = 0xA;  break;  // 0x26
      case 35:  retVal = 0xB;  break;  // 0x23
      case 52:  retVal = 0xC;  break;  // 0x34
      case 49:  retVal = 0xD;  break;  // 0x31
      case 50:  retVal = 0xE;  break;  // 0x32
      case 41:  retVal = 0xF;  break;  // 0x29
      default:                 break;  // Error
    }
    return retVal;
  }

  static bool mBusDecode(m_bus_data_t &t_in, WMbusFrame &t_frame) {
    bool retVal{false};
    if (t_in.mode == 'C') {
      if (t_in.block == 'A') {
        ESP_LOGD(TAG_LL, "Processing C1 A frame");
        std::vector<unsigned char> frame(t_in.data, t_in.data + t_in.length);
        std::string telegram = esphome::format_hex_pretty(frame);
        telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());
        ESP_LOGV(TAG_LL, "Frame: %s [with CRC]", telegram.c_str());
        if (mBusDecodeFormatA(t_in, t_frame)) {
          retVal = true;
        }
      }
      else if (t_in.block == 'B') {
        ESP_LOGD(TAG_LL, "Processing C1 B frame");
        std::vector<unsigned char> frame(t_in.data, t_in.data + t_in.length);
        std::string telegram = esphome::format_hex_pretty(frame);
        telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());
        ESP_LOGV(TAG_LL, "Frame: %s [with CRC]", telegram.c_str());
        if (mBusDecodeFormatB(t_in, t_frame)) {
          retVal = true;
        }
      }
    }
    else if (t_in.mode == 'T') {
      ESP_LOGD(TAG_LL, "Processing T1 A frame");
      std::vector<unsigned char> rawFrame(t_in.data, t_in.data + t_in.length);
      std::string telegram = esphome::format_hex_pretty(rawFrame);
      telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());
      ESP_LOGV(TAG_LL, "Frame: %s [RAW]", telegram.c_str());

      if (decode3OutOf6(&t_in, packetSize(t_in.lengthField))) {
        std::vector<unsigned char> frame(t_in.data, t_in.data + t_in.length);
        std::string telegram = esphome::format_hex_pretty(frame);
        telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());
        ESP_LOGV(TAG_LL, "Frame: %s [with CRC]", telegram.c_str());
        if (mBusDecodeFormatA(t_in, t_frame)) {
          retVal = true;
        }
      }
      else {
        ESP_LOGV(TAG_LL, "Failed to decode 3 out of 6");
      }

    }
    if (retVal) {
      std::string telegram = esphome::format_hex_pretty(t_frame.frame);
      telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());
      ESP_LOGD(TAG_LL, "Frame: %s [without CRC]", telegram.c_str());
    }
    return retVal;
  }


/*
  Format A

  L-field = length without CRC fields and without L (1 byte)

    Block 1
  ---------------------------------------------------
  | L-field | C-field | M-field | A-field |   CRC   |
  |  1 byte |  1 byte | 2 bytes | 6 bytes | 2 bytes |
  ---------------------------------------------------

    Block 2
  ---------------------------------------------------
  | CI-field |         Data-field         |   CRC   |
  |  1 byte  | 15 or (((L-9) mod 16) – 1) | 2 bytes |
  ---------------------------------------------------

    Block n (optional)
  ---------------------------------------------------
  |               Data-field              |   CRC   |
  |       16 or ((L-9) mod 16) bytes      | 2 bytes |
  ---------------------------------------------------
*/
  static bool mBusDecodeFormatA(const m_bus_data_t &t_in, WMbusFrame &t_frame) {  // in jako referencje przekazywac
    uint8_t L = t_in.data[0];

    // Validate CRC
    ESP_LOGV(TAG_LL, "Validating CRC for Block1");
    if (!crcValid(t_in.data, (BLOCK1A_SIZE - 2))) {
      return false;
    }

    // Check length of package is sufficient
    uint8_t numDataBlocks = (L - 9 + 15) / 16;                                           // Data blocks are 16 bytes long + 2 CRC bytes (not counted in L)
    if ((L < 9) || (((L - 9 + (numDataBlocks * 2))) > (t_in.length - BLOCK1A_SIZE))) {   // add CRC bytes for each data block
      ESP_LOGV(TAG_LL, "Package (%u) too short for packet Length: %u", t_in.length, L);
      ESP_LOGV(TAG_LL, "  %u > %u", (L - 9 + (numDataBlocks * 2)), (t_in.length - BLOCK1A_SIZE));
      return false;
    }

    t_frame.frame.insert(t_frame.frame.begin(), t_in.data, ( t_in.data + (BLOCK1A_SIZE - 2)));
    // Get all remaining data blocks and concatenate into data array (removing CRC bytes)
    for (uint8_t n{0}; n < numDataBlocks; ++n) {
      const uint8_t *blockStartPtr = (t_in.data + BLOCK1A_SIZE + (n * 18));  // Pointer to where data starts. Each block is 18 bytes
      uint8_t blockSize    = (MIN((L - 9 - (n * 16)), 16));                  // Maximum block size is 16 Data (without 2 CRC)

      // Validate CRC
      ESP_LOGV(TAG_LL, "Validating CRC for Block%u", (n + 2));
      if (!crcValid(blockStartPtr, (blockSize))) {
        return false;
      }

      // Get block data
      t_frame.frame.insert((t_frame.frame.begin() + ((n * 16) + BLOCK1A_SIZE - 2)), blockStartPtr, (blockStartPtr + blockSize));
    }

    return true;
  }

/*
  Format B

  L-field = length with CRC fields and without L (1 byte)

    Block 1
  ---------------------------------------------------
  |   L-field  |   C-field  |  M-field  |  A-field  |
  |    1 byte  |    1 byte  |  2 bytes  |  6 bytes  |
  ---------------------------------------------------

    Block 2
  ---------------------------------------------------
  | CI-field |         Data-field         |   CRC   |
  |  1 byte  |      115 (L-12) bytes      | 2 bytes |
  ---------------------------------------------------

    Block 3 (optional)
  ---------------------------------------------------
  |               Data-field              |   CRC   |
  |             (L-129) bytes             | 2 bytes |
  ---------------------------------------------------
*/
  static bool mBusDecodeFormatB(const m_bus_data_t &t_in, WMbusFrame &t_frame) {
    uint8_t L = t_in.data[0];
    const uint8_t *blockStartPtr{nullptr};
    uint8_t blockSize{0};

    // Check length of package is sufficient
    if ((L < 12) || ((L + 1) > t_in.length)) {  // pod len mam miec zapisane ile bajtow odebralem
      ESP_LOGV(TAG_LL, "Package (%u) too short for packet Length: %u", t_in.length, L);
      ESP_LOGV(TAG_LL, "  %u > %u", (L + 1), t_in.length);
      return false;
    }

    blockSize = MIN((L - 1), (BLOCK1B_SIZE + BLOCK2B_SIZE - 2));
    blockStartPtr = t_in.data;
    // Validate CRC for Block1 + Block2
    ESP_LOGV(TAG_LL, "Validating CRC for Block1 + Block2");
    if (!crcValid(t_in.data, blockSize)) {
      return false;
    }

    // Get data from Block1 + Block2
    t_frame.frame.insert(t_frame.frame.begin(), blockStartPtr, (blockStartPtr + blockSize));

    // Check if Block3 is present (long telegrams)
    const uint8_t L_OFFSET = (BLOCK1B_SIZE + BLOCK2B_SIZE);
    if (L > (L_OFFSET + 2)) {
      blockSize = (L - L_OFFSET - 1);
      blockStartPtr = (t_in.data + L_OFFSET);
      // Validate CRC for Block3
      ESP_LOGV(TAG_LL, "Validating CRC for Block3");
      if (!crcValid(blockStartPtr, blockSize)) {
        return false;
      }
      // Get Block3
      t_frame.frame.insert((t_frame.frame.end()), blockStartPtr, (blockStartPtr + blockSize));
    }
    return true;
  }


  static bool decode3OutOf6(uint8_t *t_encodedData, uint8_t *t_decodedData, bool t_lastByte = false) {
    uint8_t data[4];

    if (t_lastByte) {  // If last byte, ignore postamble sequence
      data[0] = 0x00;
      data[1] = 0x00;
    }
    else {  // Perform decoding on the encoded data
      data[0] = decode3of6((*(t_encodedData + 2) & 0x3F)); 
      data[1] = decode3of6(((*(t_encodedData + 2) & 0xC0) >> 6) | ((*(t_encodedData + 1) & 0x0F) << 2));
    }

    data[2] = decode3of6(((*(t_encodedData + 1) & 0xF0) >> 4) | ((*t_encodedData & 0x03) << 4));
    data[3] = decode3of6(((*t_encodedData & 0xFC) >> 2));

    // Check for possible errors
    if ( (data[0] == 0xFF) | (data[1] == 0xFF) |
        (data[2] == 0xFF) | (data[3] == 0xFF) ) {
      return false;
    }

    // Prepare decoded output
    *t_decodedData = (data[3] << 4) | (data[2]);
    if (!t_lastByte) {
      *(t_decodedData + 1) = (data[1] << 4) | (data[0]);
    }

    return true;
  } 

  static bool decode3OutOf6(m_bus_data_t *t_data,  uint16_t packetSize) {
    // We can decode "in place"
    uint8_t *encodedData = t_data->data;
    uint8_t *decodedData = t_data->data; 

    uint16_t bytesDecoded{0};
    uint16_t bytesRemaining{packetSize};

    // Decode packet      
    while (bytesRemaining) {
      // If last byte
      if (bytesRemaining == 1) {
        if(!decode3OutOf6(encodedData, decodedData, true)) {
          return false;
        }
        bytesRemaining -= 1;
        bytesDecoded   += 1;
      }
      else {
        if(!decode3OutOf6(encodedData, decodedData)) {
          return false;
        }
        bytesRemaining -= 2;
        bytesDecoded   += 2;

        encodedData += 3;
        decodedData += 2;
      }
    }
    t_data->length = bytesDecoded;
    std::fill((std::begin(t_data->data) + t_data->length), std::end(t_data->data), 0);
    return true;
  }

  static uint16_t byteSize(uint16_t t_packetSize) {
    // In T-mode data is 3 out of 6 coded.
    uint16_t size = (( 3 * t_packetSize) / 2);

    // If packetsize is a odd number 1 extra byte   
    // that includes the 4-postamble sequence must be
    // read.    
    if (t_packetSize % 2) {
      return (size + 1);
    }
    else {
      return (size);
    }
  }

  static uint16_t packetSize(uint8_t t_L) {
    uint16_t nrBytes;
    uint8_t  nrBlocks;

    // The 2 first blocks contains 25 bytes when excluding CRC and the L-field
    // The other blocks contains 16 bytes when excluding the CRC-fields
    // Less than 26 (15 + 10) 
    if ( t_L < 26 ) {
      nrBlocks = 2;
    }
    else { 
      nrBlocks = (((t_L - 26) / 16) + 3);
    }

    // Add all extra fields, excluding the CRC fields
    nrBytes = t_L + 1;

    // Add the CRC fields, each block is contains 2 CRC bytes
    nrBytes += (2 * nrBlocks);

    return nrBytes;
  }

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