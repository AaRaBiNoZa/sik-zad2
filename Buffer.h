//
// Created by ciriubuntu on 12.05.22.
//

#ifndef SIK_ZAD3_BUFFER_H
#define SIK_ZAD3_BUFFER_H

#include <boost/asio.hpp>
#include <utility>
#include <vector>

#include "common.h"
using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

class BufferException: public std::exception {};

class UdpOverflowException: public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "UDP buffer overflow";
  }
};

class MessageTooShortException: public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "Unserialize expected more";
  }
};

class MessageTooLongException: public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "Unserialize expected more";
  }
};

class ConnectionAborted: public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "Connection closed by the peer";
  }
};

class StreamBuffer {
 private:
  std::vector<uint8_t> buff;
 public:
  virtual void getNBytes(uint8_t n, std::vector<uint8_t>& data) = 0;
  virtual void endRec() = 0;
  virtual void reset() = 0;
  virtual void send() = 0;
  virtual void get() = 0;
  virtual void writeNBytes(uint8_t n, std::vector<uint8_t> buff) = 0;
  virtual ~StreamBuffer() = default;
};

class TcpStreamBuffer : public StreamBuffer {
 private:
  static const uint16_t max_data_size = 65535;
  std::shared_ptr<boost::asio::ip::tcp::socket> sock;
  std::vector<uint8_t> buff;
  size_t ptr_to_act_el{};

 public:
  explicit TcpStreamBuffer(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
      : sock(std::move(sock)), buff(max_data_size){};

  void getNBytes(uint8_t n, std::vector<uint8_t>& data) override {
    try {
      boost::asio::read(*sock, boost::asio::buffer(data, n));
    } catch (...) {
      throw ConnectionAborted();
    }
  }

  void send() override {
    if (ptr_to_act_el != 0) {
      sock->send(boost::asio::buffer(buff, ptr_to_act_el));
      ptr_to_act_el = 0;
    }

  }

  void endRec() override {
    return;
  }

  void get() override {
    return;
  }

  void reset() override {
    ptr_to_act_el = 0;
  }
  void writeNBytes(uint8_t n, std::vector<uint8_t> buffer) override {
    memcpy(&buff[ptr_to_act_el], &buffer[0], n);
    ptr_to_act_el += n;
  }

};

class UdpStreamBuffer : public StreamBuffer {
 private:
  static const uint16_t max_data_size = 65507;
  std::shared_ptr<boost::asio::ip::udp::socket> sock;
  std::vector<uint8_t> buff;
  size_t ptr_to_act_el{};

  size_t len{};

 public:
  explicit UdpStreamBuffer(std::shared_ptr<boost::asio::ip::udp::socket>  sock)
      : sock(std::move(sock)), buff(max_data_size) {

  };
  void reset() override {
    ptr_to_act_el = 0;
  }

  void get() override {
    len = sock->receive(boost::asio::buffer(buff));
  }

  void getNBytes(uint8_t n, std::vector<uint8_t>& data) override {
    if (ptr_to_act_el + n > len) {
      throw MessageTooShortException();
    }

    for (size_t i = 0; i < n; ++i) {
      data[i] = buff[ptr_to_act_el];
      ptr_to_act_el++;
    }
  }

  void endRec() override {
    if (len != ptr_to_act_el) {
      throw MessageTooLongException();
    }
  }

  void writeNBytes(uint8_t n, std::vector<uint8_t> buffer) override {
    if (ptr_to_act_el + n > max_data_size) {
      throw UdpOverflowException();
    }

    memcpy(&buff[ptr_to_act_el], &buffer[0], n);
    ptr_to_act_el += n;
  }
  void send() override {
    if (ptr_to_act_el != 0) {
      sock->send(boost::asio::buffer(buff, ptr_to_act_el));
    }
  }

  ~UdpStreamBuffer() override = default;
};

#endif  // SIK_ZAD3_BUFFER_H
