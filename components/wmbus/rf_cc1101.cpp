#include "rf_cc1101.h"

namespace esphome {
namespace wmbus {

  static const char *TAG = "rxLoop";

  bool RxLoop::init(uint8_t mosi, uint8_t miso, uint8_t clk, uint8_t cs,
                    uint8_t gdo0, uint8_t gdo2, float freq, bool syncMode) {
    bool retVal = false;
    this->syncMode = syncMode;
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

    ESP_LOGD(TAG, "Set CC1101 frequency to %3.3fMHz [%02X %02X %02X]",
             freq/1000000, freq2, freq1, freq0);
             // don't use setMHZ() -- seems to be broken, or used in wrong place
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ2, freq2);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ1, freq1);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ0, freq0);

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SCAL);

    byte cc1101Version = ELECHOUSE_cc1101.SpiReadStatus(CC1101_VERSION);

    if ((cc1101Version != 0) && (cc1101Version != 255)) {
      retVal = true;
      ESP_LOGD(TAG, "CC1101 version '%d'", cc1101Version);
      ELECHOUSE_cc1101.SetRx();
      ESP_LOGD(TAG, "CC1101 initialized");
      delay(4);
    }
    else {
      ESP_LOGE(TAG, "CC1101 initialization FAILED!");
    }

    return retVal;
  }

  bool RxLoop::task() {
    do {
      // The radio's byte count, read once a pass and used everywhere below: how full the buffer is,
      // whether it has overflowed, and whether the telegram is all in. One read a pass, which is
      // what this loop always did, so there is no more chatter on the wire than before.
      uint8_t rxStatus = ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES);
      uint8_t overfl   = rxStatus & 0x80;
      uint8_t inFifo   = rxStatus & 0x7F;
      // The chip can hand back a count caught mid-update, so only a figure two reads running agree
      // on is acted on (CC1101 errata SWRZ020E).
      bool settled = (inFifo == this->last_in_fifo_);
      this->last_in_fifo_ = inFifo;

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
            rxLoop.bytesRx = 3;
            const uint8_t *currentByte = rxLoop.pByteIndex;
            // Mode C
            if (*currentByte == WMBUS_MODE_C_PREAMBLE) {
              currentByte++;
              data_in.mode = 'C';
              // Block A
              if (*currentByte == WMBUS_BLOCK_A_PREAMBLE) {
                currentByte++;
                rxLoop.lengthField = *currentByte;
                rxLoop.length = 2 + packetSize(rxLoop.lengthField);
                data_in.block = 'A';
              }
              // Block B
              else if (*currentByte == WMBUS_BLOCK_B_PREAMBLE) {
                currentByte++;
                rxLoop.lengthField = *currentByte;
                rxLoop.length = 2 + 1 + rxLoop.lengthField;
                data_in.block = 'B';
              }
              // Unknown type, reinit loop
              else {
                rxLoop.state = INIT_RX;
                return false;
              }
              // don't include C "preamble"
              *(rxLoop.pByteIndex) = rxLoop.lengthField;
              rxLoop.pByteIndex += 1;
            }
            // Mode T Block A
            else if (decode3OutOf6(rxLoop.pByteIndex, preamble)) {
              rxLoop.lengthField  = preamble[0];
              data_in.lengthField = rxLoop.lengthField;
              rxLoop.length  = byteSize(packetSize(rxLoop.lengthField));
              data_in.mode   = 'T';
              data_in.block  = 'A';
              rxLoop.pByteIndex += 3;
            }
            // Unknown mode, reinit loop
            else {
              rxLoop.state = INIT_RX;
              return false;
            }

            rxLoop.bytesLeft = rxLoop.length - 3;
            rxLoop.state = READ_DATA;
            max_wait_time_ += extra_time_;

            // The header is now known, and with it how many bytes are still to come. Nothing is
            // written back to the radio here.
            //
            // This is the whole of the fix. Switching PKTCTRL0 from infinite to fixed length in the
            // middle of a telegram makes the CC1101 de-assert its end-of-packet output straight
            // away, and the loop below then reads the entire rest of the telegram out of a buffer
            // holding one to eight bytes. Nothing is wrong with what the radio received: the
            // telegram is assembled out of whatever an under-read FIFO returns. So stay in
            // infinite-packet mode for the whole reception and find the end by counting bytes,
            // which the length field in the header has just told us.
            //
            // The count read at the top of this pass is three bytes stale now, so throw it away and
            // let the pass end here.
            this->last_in_fifo_ = 0xFF;
            continue;
          }
          break;

        // waiting for more data in Rx FIFO buffer
        case READ_DATA:
          // Only reach into the buffer while the telegram is still arriving if it would otherwise
          // overflow. Anything that fits in the chip's 64-byte buffer — every wM-Bus telegram up to
          // about forty bytes of payload, which is most of them — is collected in one go below,
          // after the transmission is over.
          if (settled && (inFifo >= RX_FIFO_DRAIN_AT) && (rxLoop.bytesLeft > inFifo)) {
            // Do not empty the Rx FIFO (See the CC1101 SWRZ020E errata note)
            uint8_t chunk = inFifo - 1;
            ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, rxLoop.pByteIndex, chunk);
            rxLoop.bytesLeft  -= chunk;
            rxLoop.pByteIndex += chunk;
            rxLoop.bytesRx    += chunk;
            max_wait_time_    += extra_time_;
            this->last_in_fifo_ = 0xFF;
          }
          break;
      }

      // The rest of the telegram is sitting in the buffer, so the transmission is over. The second
      // read is the errata's own answer to a byte count caught mid-update, and it costs nothing that
      // matters: by the time it happens the telegram is already in.
      if ((!overfl) && (rxLoop.state > WAIT_FOR_DATA) && (inFifo >= rxLoop.bytesLeft) &&
          ((ELECHOUSE_cc1101.SpiReadStatus(CC1101_RXBYTES) & 0x7F) >= rxLoop.bytesLeft)) {
        ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_RXFIFO, rxLoop.pByteIndex, (uint8_t)rxLoop.bytesLeft);
        rxLoop.bytesRx += rxLoop.bytesLeft;
        data_in.length  = rxLoop.bytesRx;
        this->returnFrame.rssi  = (int8_t)ELECHOUSE_cc1101.getRssi();
        this->returnFrame.lqi   = (uint8_t)ELECHOUSE_cc1101.getLqi();
        ESP_LOGV(TAG, "Have %d bytes from CC1101 Rx, RSSI: %d dBm LQI: %d", rxLoop.bytesRx, this->returnFrame.rssi, this->returnFrame.lqi);
        if (rxLoop.length != data_in.length) {
          ESP_LOGE(TAG, "Length problem: req(%d) != rx(%d)", rxLoop.length, data_in.length);
        }
        if (this->syncMode) {
          ESP_LOGV(TAG, "Synchronus mode enabled.");
        }
        if (mBusDecode(data_in, this->returnFrame)) {
          rxLoop.complete = true;
          this->returnFrame.mode  = data_in.mode;
          this->returnFrame.block = data_in.block;
        }
        rxLoop.state = INIT_RX;
        return rxLoop.complete;
      }
      start(false);
    } while ((this->syncMode) && (rxLoop.state > WAIT_FOR_SYNC));
    return rxLoop.complete;
  }

  WMbusFrame RxLoop::get_frame() {
    return this->returnFrame;
  }

  bool RxLoop::start(bool force) {
    // waiting to long for next part of data?
    bool reinit_needed = ((millis() - sync_time_) > max_wait_time_) ? true: false;
    if (!force) {
      if (!reinit_needed) {
        // already in RX?
        if (ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) == MARCSTATE_RX) {
          return false;
        }
      }
    }
    // init RX here, each time we're idle
    rxLoop.state = INIT_RX;
    sync_time_ = millis();
    max_wait_time_ = extra_time_;

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    while((ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) != MARCSTATE_IDLE));
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);  //flush Tx FIFO
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFRX);  //flush Rx FIFO

    // Initialize RX info variable
    rxLoop.lengthField = 0;              // Length Field in the wM-Bus packet
    rxLoop.length      = 0;              // Total length of bytes to receive packet
    rxLoop.bytesLeft   = 0;              // Bytes left to to be read from the Rx FIFO
    rxLoop.bytesRx     = 0;              // Bytes read from Rx FIFO
    rxLoop.pByteIndex  = data_in.data;   // Pointer to current position in the byte array
    rxLoop.complete    = false;          // Packet received
    this->last_in_fifo_ = 0xFF;          // No byte count seen yet for this packet

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

    // Set Rx FIFO threshold to 4 bytes
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FIFOTHR, RX_FIFO_START_THRESHOLD);
    // Set infinite length 
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, INFINITE_PACKET_LENGTH);

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SRX);
    while((ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) != MARCSTATE_RX));

    rxLoop.state = WAIT_FOR_SYNC;

    return true; // this will indicate we just have re-started Rx
  }

}
}