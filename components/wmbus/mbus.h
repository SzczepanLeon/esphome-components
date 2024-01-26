#pragma once

#include "esphome/core/log.h"
#include "m_bus_data.h"
#include "utils.h"

namespace esphome {
namespace wmbus {

bool mBusDecode(m_bus_data_t &t_in, WMbusFrame &t_frame);
bool mBusDecodeFormatA(const m_bus_data_t &t_in, WMbusFrame &t_frame);
bool mBusDecodeFormatB(const m_bus_data_t &t_in, WMbusFrame &t_frame);

}
}