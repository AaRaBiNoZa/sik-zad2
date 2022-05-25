#ifndef SIK_ZAD3_BYTESTREAM_H
#define SIK_ZAD3_BYTESTREAM_H

#include <netinet/in.h>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <memory>

#include "Buffer.h"
#include "common.h"

/**
 * It is a class that provides an easy interface for an associated buffer
 * that is supposed to be receiving/sending messages.
 * It stores its own local vector needed for single datatype read/writes from
 * an associated buffer.
 */
class ByteStream {
 private:
  std::vector<uint8_t> data;
  std::unique_ptr<StreamBuffer> buffer;

 public:

  /**
   * Since all read/write operations here are meant for single datatype,
   * we can initialize the data vector with a size of max_single_datatype_size
   * @param buff
   */
  explicit ByteStream(std::unique_ptr<StreamBuffer> buff)
      : data(max_single_datatype_size),
        buffer(std::move(buff)){};

  /**
   * used to prepare the underlying buffer to read/write
   */
  void reset() {
    buffer->reset();
  }

  /**
   * If underlying system is a single message-based system, then
   * this function reads the whole message to buffer's own buffer.
   * Else UB.
   */
  void get() {
    buffer->get();
  }

  /**
   * Does everything necessary after receiving/reading from underlying system
   * (in this case it's a socket).
   */
  void endRec() {
    buffer->endRec();
  }

  /**
   * Does everything necessary after writing to underlying system
   * (in this case it's a socket).
   */
  void endWrite() {
    buffer->send();
  }

  /**
   * Here are overloaded operators that provide an extremely easy interface
   * to work with this object.
   */
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
    x = (char) data[0];
    return *this;
  }
  ByteStream& operator<<(char x) {
    data[0] = (uint8_t) x;
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
    auto length = (uint8_t) s.size();
    *this << length;
    std::memcpy(&data[0], s.data(), length);
    buffer->writeNBytes(length, data);
    return *this;
  }

  template <typename T>
  ByteStream& operator>>(std::vector<T>& x) {
    uint32_t len;
    *this >> len;
    x.resize(len);
    for (size_t i = 0; i < len; ++i) {
      *this >> x[i];
    }
    return *this;
  }
  template <typename T>
  ByteStream& operator<<(std::vector<T> x) {
    auto len = (uint32_t) x.size();
    *this << len;
    for (size_t i = 0; i < len; ++i) {
      *this << x[i];
    }
    return *this;
  }

  template <typename T>
  ByteStream& operator>>(std::set<T>& x) {
    uint32_t len;
    *this >> len;
    T temp;
    for (size_t i = 0; i < len; ++i) {
      *this >> temp;
      x.insert(temp);
    }
    return *this;
  }
  template <typename T>
  ByteStream& operator<<(std::set<T> x) {
    auto len = (uint32_t) x.size();
    *this << len;
    for (auto element: x) {
      *this << element;
    }
    return *this;
  }

  template <typename T1, typename T2>
  ByteStream& operator>>(std::map<T1, T2>& x) {
    uint32_t len;
    *this >> len;
    x.clear();
    std::pair<T1, T2> temp_val;
    for (size_t i = 0; i < len; ++i) {
      *this >> temp_val;
      x.insert(temp_val);
    }
    return *this;
  }
  template <typename T1, typename T2>
  ByteStream& operator<<(std::map<T1, T2> x) {
    auto len = (uint32_t) x.size();
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
