#pragma once
#include <string>
#include <vector>

#include "esphome/components/socket/socket.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace socket_transmitter {
static const char *TAG = "socket_transmitter";

class SocketTransmitter : public Component {
public:
  void set_host(std::string host) { this->host = host; };
  void set_port(int port) { this->port = port; };
  void set_protocol(int protocol) { this->protocol = protocol; };
  void send(std::string data);
  void send(std::vector<uint8_t> data);
  void send(const uint8_t *data, size_t length);
  void dump_config() override;
  float get_setup_priority() const override {
    return setup_priority::AFTER_CONNECTION;
  }

protected:
  std::string host;
  int port;
  int protocol;
  std::unique_ptr<socket::Socket> socket_;
};

template <typename StrOrVector, typename... Ts>
class SocketTransmitterSendAction : public Action<Ts...> {
public:
  SocketTransmitterSendAction(SocketTransmitter *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(StrOrVector, data)

  void play(const Ts& ... x) override {
    this->parent_->send(this->data_.value(x...));
  }

protected:
  SocketTransmitter *parent_;
};
} // namespace socket_transmitter
} // namespace esphome
