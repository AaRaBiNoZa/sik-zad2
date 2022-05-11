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
#include <vector>

class ByteStream {
 private:
  std::vector<uint8_t> data;
  uint8_t ptr_to_act_el;

 public:
  ByteStream() : data(256), ptr_to_act_el(0){};

  std::vector<uint8_t>& getData() {
    return data;
  }
  void resetPtr() {
    ptr_to_act_el = 0;
  }

  ByteStream& operator<<(uint8_t x) {
    data[ptr_to_act_el] = x;
    ptr_to_act_el += sizeof(x);
    return *this;
  }
  ByteStream& operator>>(uint8_t& x) {
    x = data[ptr_to_act_el];
    ptr_to_act_el += sizeof(x);
    return *this;
  }

  ByteStream& operator<<(uint16_t x) {
    x = htons(x);
    std::memcpy(&data[ptr_to_act_el], &x, sizeof(x));
    ptr_to_act_el += sizeof(x);
    return *this;
  }
  ByteStream& operator>>(uint16_t& x) {
    std::memcpy(&x, &data[ptr_to_act_el], sizeof(x));
    x = ntohs(x);
    ptr_to_act_el += sizeof(x);
    return *this;
  }

  ByteStream& operator<<(uint64_t x) {
    x = htobe64(x);
    std::memcpy(&data[ptr_to_act_el], &x, sizeof(x));
    ptr_to_act_el += sizeof(x);
    return *this;
  }
  ByteStream& operator>>(uint64_t& x) {
    std::memcpy(&x, &data[ptr_to_act_el], sizeof(x));
    x = be64toh(x);
    ptr_to_act_el += sizeof(x);
    return *this;
  }

  ByteStream& operator<<(uint32_t x) {
    x = htonl(x);
    std::memcpy(&data[ptr_to_act_el], &x, sizeof(x));
    ptr_to_act_el += sizeof(x);
    return *this;
  }
  ByteStream& operator>>(uint32_t& x) {
    std::memcpy(&x, &data[ptr_to_act_el], sizeof(x));
    x = ntohl(x);
    ptr_to_act_el += sizeof(x);
    return *this;
  }

  ByteStream& operator>>(char& x) {
    x = data[ptr_to_act_el];
    ptr_to_act_el += sizeof(x);
    return *this;
  }
  ByteStream& operator<<(char x) {
    data[ptr_to_act_el] = x;
    ptr_to_act_el += sizeof(x);
    return *this;
  }
  ByteStream& operator>>(std::string& s) {
    uint8_t length;
    *this >> length;
    s.resize(length);
    std::memcpy(s.data(), &data[ptr_to_act_el], s.size());
    ptr_to_act_el += s.length();
    return *this;
  }
  ByteStream& operator<<(std::string s) {
    uint8_t length = s.size();
    *this << length;
    std::memcpy(&data[ptr_to_act_el], s.data(), length);
    ptr_to_act_el += s.length();
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
    for (auto& it: x) {
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
