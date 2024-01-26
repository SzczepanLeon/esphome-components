#pragma once

#include "esphome/core/log.h"
#include "m_bus_data.h"
#include "utils.h"
#include <vector>

#define BLOCK1A_SIZE 12     // Size of Block 1, format A
#define BLOCK1B_SIZE 10     // Size of Block 1, format B
#define BLOCK2B_SIZE 118    // Maximum size of Block 2, format B

namespace esphome {
namespace wmbus {

bool mBusDecode(m_bus_data_t &t_in, WMbusFrame &t_frame);
bool mBusDecodeFormatA(const m_bus_data_t &t_in, WMbusFrame &t_frame);
bool mBusDecodeFormatB(const m_bus_data_t &t_in, WMbusFrame &t_frame);

}
}