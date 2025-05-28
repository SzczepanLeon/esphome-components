#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "wmbus_meter.h"

namespace esphome {
namespace wmbus_meter {
class BaseSensor : public Parented<Meter>, public Component {
public:
  void set_field_name(std::string field_name);
  virtual void handle_update() = 0;
  void set_parent(Meter *parent);

protected:
  std::string field_name;
};
} // namespace wmbus_meter
} // namespace esphome