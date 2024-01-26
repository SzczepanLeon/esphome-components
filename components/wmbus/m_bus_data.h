#pragma once

namespace esphome {
namespace wmbus {

  typedef struct {
    uint16_t  length;
    uint8_t   lengthField;
    uint8_t   data[500];
    char      mode;
    char      block;
  } m_bus_data_t;


}
}