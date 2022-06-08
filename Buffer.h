#ifndef SIK_ZAD3_BUFFER_H
#define SIK_ZAD3_BUFFER_H

#include <boost/asio.hpp>
#include <utility>
#include <vector>

#include "utils.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

/**
 * Basic exceptions, names are self-explanatory
 */
class BufferException : public std::exception {};

class UdpOverflowException : public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "UDP buffer overflow";
  }
};

class MessageTooShortException : public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "deserialize expected more";
  }
};

class MessageTooLongException : public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "deserialize expected more";
  }
};

class ConnectionAborted : public BufferException {
  [[nodiscard]] const char* what() const noexcept override {
    return "Connection closed by the peer";
  }
};

/**
 * This is an interface for the next two classes.
 * The idea is to have a single entity with an easy interface
 * for both tcp and udp.
 * Surely - it's not that easy, that's why some methods needed by
 * udp version are not really needed by tcp version and vice versa, but
 * thanks to that we get a common interface.
 */
class StreamBuffer {
 private:
  std::vector<uint8_t> buff;

 public:
  // max_len string
  static const uint16_t max_single_datatype_size = 256;

  virtual void get_n_bytes(uint8_t n, std::vector<uint8_t>& data) = 0;
  virtual void
  end_receive() = 0;         // used for cleaning after any type of read/receive
  virtual void reset() = 0;  // preparation for read/send
  virtual void send() = 0;
  virtual void get() = 0;  // for udp only - reads a full message
  virtual void write_n_bytes(uint8_t n, std::vector<uint8_t> buff) = 0;

  virtual ~StreamBuffer() = default;
};

class TcpStreamBuffer : public StreamBuffer {
 private:
  std::shared_ptr<boost::asio::ip::tcp::socket> sock;
  std::vector<uint8_t> internal_buffer;
  size_t bytes_to_send_count{};

 public:
  explicit TcpStreamBuffer(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
      : sock(std::move(sock)), internal_buffer(max_single_datatype_size){};

  explicit TcpStreamBuffer() : internal_buffer(max_single_datatype_size){};

  void get_n_bytes(uint8_t n, std::vector<uint8_t>& data) override {
    try {
      boost::asio::read(*sock, boost::asio::buffer(data, n));
    } catch (...) {
      throw ConnectionAborted();
    }
  }

  void send() override {
    if (bytes_to_send_count != 0) {
      sock->send(boost::asio::buffer(internal_buffer, bytes_to_send_count));
      bytes_to_send_count = 0;
    }
  }

  void end_receive() override {
  }

  void get() override {
  }

  void reset() override {
    bytes_to_send_count = 0;
  }
  void write_n_bytes(uint8_t n, std::vector<uint8_t> buffer) override {
    if (bytes_to_send_count + n > max_single_datatype_size) {
      send();
    }
    memcpy(&internal_buffer[bytes_to_send_count], &buffer[0], n);
    bytes_to_send_count += n;
  }

  ~TcpStreamBuffer() override = default;
};

/**
 * I need to create two sockets here - one that I can connect, for sending
 * ( so when I send and the receiver is not receiving, I can get the info )
 * And another for receiving, since I should be prepared to receive
 * messages from any possible address.
 * Connecting UDP socket is handd because it takes care of ICMP
 * unreachable messages and can detect problems with GUI.
 */
class UdpStreamBuffer : public StreamBuffer {
 private:
  static const uint16_t max_data_size = 65507;
  std::shared_ptr<boost::asio::ip::udp::socket> rec_sock;
  boost::asio::ip::udp::socket send_sock;
  boost::asio::ip::udp::endpoint remote_endpoint;
  std::vector<uint8_t> internal_buff;
  size_t bytes_to_send_count{};

  size_t len{};

 public:
  explicit UdpStreamBuffer(std::shared_ptr<boost::asio::ip::udp::socket> sock,
                           const std::string& remote_address,
                           boost::asio::io_context& io_context)
      : rec_sock(std::move(sock)),
        send_sock(io_context, udp::endpoint(udp::v6(), 0)),
        internal_buff(max_data_size) {
    auto [remote_host, remote_port] = extract_host_and_port(remote_address);
    udp::resolver udp_resolver(io_context);
    remote_endpoint = *udp_resolver.resolve(udp::v6(), remote_host, remote_port,
                                            resolver_base::numeric_service |
                                                resolver_base::v4_mapped |
                                                resolver_base::all_matching);

    send_sock.connect(remote_endpoint);
  };
  void reset() override {
    bytes_to_send_count = 0;
  }

  void get() override {
    boost::asio::ip::udp::endpoint dummy;
    len = rec_sock->receive_from(boost::asio::buffer(internal_buff), dummy);
  }

  void get_n_bytes(uint8_t n, std::vector<uint8_t>& data) override {
    if (bytes_to_send_count + n > len) {
      throw MessageTooShortException();
    }

    for (size_t i = 0; i < n; ++i) {
      data[i] = internal_buff[bytes_to_send_count];
      bytes_to_send_count++;
    }
  }

  void end_receive() override {
    if (len != bytes_to_send_count) {
      throw MessageTooLongException();
    }
  }

  void write_n_bytes(uint8_t n, std::vector<uint8_t> buffer) override {
    if (bytes_to_send_count + n > max_data_size) {
      throw UdpOverflowException();
    }

    memcpy(&internal_buff[bytes_to_send_count], &buffer[0], n);
    bytes_to_send_count += n;
  }
  void send() override {
    if (bytes_to_send_count != 0) {
      send_sock.send(boost::asio::buffer(internal_buff, bytes_to_send_count));
    }
  }

  ~UdpStreamBuffer() override = default;
};

#endif  // SIK_ZAD3_BUFFER_H
