#include "socket_transmitter.h"

namespace esphome {
namespace socket_transmitter {
void SocketTransmitter::send(std::string data) {
  return this->send((uint8_t *)data.c_str(), data.length());
}

void SocketTransmitter::send(std::vector<uint8_t> data) {
  return this->send(data.data(), data.size());
}

void SocketTransmitter::send(const uint8_t *data, size_t length) {
  ESP_LOGD(TAG, "Setting up socket transmitter");
  this->socket_ = socket::socket_ip(this->protocol, 0);
  int enable = 1;
  this->socket_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  ESP_LOGD(TAG, "Connecting %s ...",
           this->protocol == SOCK_DGRAM ? "UDP" : "TCP");
  sockaddr destination;
  socket::set_sockaddr(&destination, sizeof(destination), this->host,
                       this->port);
  if (this->socket_->connect(&destination, sizeof(destination)) < 0) {
    ESP_LOGE(TAG, "Failed to connect");
    return;
  }

  ESP_LOGD(TAG, "Sending frame [%d bytes]", length);
  int n_bytes = this->socket_->write(data, length);
  if (n_bytes < 0)
    ESP_LOGE(TAG, "Failed to send message");
  this->socket_->close();
}

void SocketTransmitter::dump_config() {
  auto protocol = this->protocol == SOCK_DGRAM ? "UDP" : "TCP";

  ESP_LOGCONFIG(TAG, "Socket Transmitter:");
  ESP_LOGCONFIG(TAG, "  Destination: %s:%d", this->host.c_str(), this->port);
  ESP_LOGCONFIG(TAG, "  Protocol: %s", protocol);
}
} // namespace socket_transmitter
} // namespace esphome
