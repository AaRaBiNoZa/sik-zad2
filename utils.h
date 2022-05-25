#ifndef SIK_ZAD3_UTILS_H
#define SIK_ZAD3_UTILS_H

#include <string>

#include "ByteStream.h"

/**
 * Smaller utility classes and using directives.
 */

using PlayerId = uint8_t;
using BombId = uint32_t;
using Score = uint32_t;

class Player {
 public:
  std::string name;
  std::string address;
  Player(std::string name, std::string address)
      : name(std::move(name)), address(std::move(address)){};
  Player() : name(), address(){};
  friend ByteStream& operator<<(ByteStream& os, Player& player) {
    os << player.name;
    os << player.address;
    return os;
  }

  friend ByteStream& operator>>(ByteStream& os, Player& player) {
    os >> player.name;
    os >> player.address;
    return os;
  }
};

class Position {
 public:
  uint16_t x{};
  uint16_t y{};
  Position(uint16_t x, uint16_t y) : x(x), y(y){};
  Position() = default;

  friend ByteStream& operator<<(ByteStream& os, Position& position) {
    os << position.x;
    os << position.y;
    return os;
  }

  friend ByteStream& operator>>(ByteStream& os, Position& position) {
    os >> position.x;
    os >> position.y;
    return os;
  }

  bool operator<(const Position& pos2) const {
    if (x < pos2.x) {
      return true;
    } else if (x == pos2.x) {
      return y < pos2.y;
    }
    return false;
  }
};

class Bomb {
 private:
 public:
  Position position;
  uint16_t timer;

  Bomb(Position pos, uint16_t timer) : position(pos), timer(timer){};
  Bomb() : position(), timer(){};
  friend ByteStream& operator<<(ByteStream& os, Bomb& bomb) {
    os << bomb.position;
    os << bomb.timer;
    return os;
  }

  friend ByteStream& operator>>(ByteStream& os, Bomb& bomb) {
    os >> bomb.position;
    os >> bomb.timer;
    return os;
  }

  bool operator<(const Bomb& bomb2) const {
    return position < bomb2.position;
  }
};

#endif  // SIK_ZAD3_UTILS_H
