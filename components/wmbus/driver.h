#pragma once

#include <vector>

struct Driver
{
  virtual ~Driver() = default; 

public:
  virtual bool get_value(std::vector<unsigned char> &telegram, float &total_usage) = 0;
  std::string get_name() { return this->driver_type_; };

protected:
  Driver(std::string driver_type) : driver_type_(driver_type) {};

private:
  Driver();
  std::string driver_type_;
};