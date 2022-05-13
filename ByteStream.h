//
// Created by Adam Al-Hosam on 05/05/2022.
//

#ifndef SIK_ZAD3_BYTESTREAM_H
#define SIK_ZAD3_BYTESTREAM_H

#include <netinet/in.h>

#include <cstring>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "Buffer.h"

class ByteStream {
 private:
  std::vector<uint8_t> data;
  std::unique_ptr<StreamBuffer> buffer;
  uint8_t ptr_to_act_el;

 public:
//  explicit ByteStream(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
//      : data(256),
//        buffer(std::make_shared<TcpStreamBuffer>(socket)),
//        ptr_to_act_el(0){};
  explicit ByteStream(std::unique_ptr<StreamBuffer> buff)
      : data(256),
        buffer(std::move(buff)),
        ptr_to_act_el(0){};
  //  explicit ByteStream() = default;

  std::vector<uint8_t>& getData() {
    return data;
  }
  void resetPtr() {
    ptr_to_act_el = 0;
  }

  void reset() {
    buffer->reset();
  }

  void get() {
    buffer->get();
  }

  void endRec() {
    buffer->endRec();
  }

  void endWrite() {
    buffer->send();
  }

  ByteStream& operator<<(uint8_t x) {
    data[0] = x;
    buffer->writeNBytes(sizeof(x), data);
    return *this;
  }
  ByteStream& operator>>(uint8_t& x) {
    buffer->getNBytes(sizeof(x), data);
    x = data[0];
    return *this;
  }

  ByteStream& operator<<(uint16_t x) {
    x = htons(x);
    std::memcpy(&data[0], &x, sizeof(x));
    buffer->writeNBytes(sizeof(x), data);
    return *this;
  }
  ByteStream& operator>>(uint16_t& x) {
    buffer->getNBytes(sizeof(x), data);
    std::memcpy(&x, &data[0], sizeof(x));
    x = ntohs(x);
    return *this;
  }

  ByteStream& operator<<(uint64_t x) {
    x = htobe64(x);
    std::memcpy(&data[0], &x, sizeof(x));
    buffer->writeNBytes(sizeof(x), data);
    return *this;
  }
  ByteStream& operator>>(uint64_t& x) {
    buffer->getNBytes(sizeof(x), data);
    std::memcpy(&x, &data[0], sizeof(x));
    x = be64toh(x);
    return *this;
  }

  ByteStream& operator<<(uint32_t x) {
    x = htonl(x);
    std::memcpy(&data[0], &x, sizeof(x));
    buffer->writeNBytes(sizeof(x), data);
    return *this;
  }
  ByteStream& operator>>(uint32_t& x) {
    buffer->getNBytes(sizeof(x), data);
    std::memcpy(&x, &data[0], sizeof(x));
    x = ntohl(x);
    return *this;
  }

  ByteStream& operator>>(char& x) {
    buffer->getNBytes(sizeof(x), data);
    x = data[0];
    return *this;
  }
  ByteStream& operator<<(char x) {
    data[0] = x;
    buffer->writeNBytes(sizeof(x), data);

    return *this;
  }
  ByteStream& operator>>(std::string& s) {
    uint8_t length;
    *this >> length;
    s.resize(length);
    buffer->getNBytes(length, data);
    std::memcpy(s.data(), &data[0], s.size());
    return *this;
  }
  ByteStream& operator<<(std::string s) {
    uint8_t length = s.size();
    *this << length;
    std::memcpy(&data[0], s.data(), length);
    buffer->writeNBytes(s.size(), data);
    return *this;
  }

  template <typename T>
  ByteStream& operator>>(std::vector<T>& x) {
    uint32_t len;
    *this >> len;
    x.resize(len);
    for (int i = 0; i < len; ++i) {
      *this >> x[i];
    }
    return *this;
  }
  template <typename T>
  ByteStream& operator<<(std::vector<T> x) {
    uint32_t len = x.size();
    *this << len;
    for (int i = 0; i < len; ++i) {
      *this << x[i];
    }
    return *this;
  }

  template <typename T1, typename T2>
  ByteStream& operator>>(std::map<T1, T2>& x) {
    uint32_t len;
    *this >> len;
    x.clear();
    std::pair<T1, T2> temp_val;
    for (int i = 0; i < len; ++i) {
      *this >> temp_val;
      x.insert(temp_val);
    }
    return *this;
  }
  template <typename T1, typename T2>
  ByteStream& operator<<(std::map<T1, T2> x) {
    uint32_t len = x.size();
    *this << len;
    for (auto& it : x) {
      *this << it;
    }
    return *this;
  }

  template <typename T1, typename T2>
  ByteStream& operator>>(std::pair<T1, T2>& x) {
    *this >> x.first;
    *this >> x.second;
    return *this;
  }
  template <typename T1, typename T2>
  ByteStream& operator<<(std::pair<T1, T2> x) {
    *this << x.first;
    *this << x.second;
    return *this;
  }
};

#endif  // SIK_ZAD3_BYTESTREAM_H
