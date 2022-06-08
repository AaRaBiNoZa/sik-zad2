#ifndef SIK_ZAD3_UTILS_H
#define SIK_ZAD3_UTILS_H

#include <string>

#include "ByteStream.h"

/**
 * Used during setup - when given a string with colons inside,
 * returns two strings divided by the last colon.
 */
std::pair<std::string, std::string> extract_host_and_port(
    const std::string& addr) {
  size_t dividor_position = addr.rfind(':');
  std::string clean_host = addr.substr(0, dividor_position);
  std::string clean_port = addr.substr(dividor_position + 1);

  return {clean_host, clean_port};
}

/**
 * Smaller utility classes and using directives.
 */

using PlayerId = uint8_t;
using BombId = uint32_t;
using Score = uint32_t;

struct PlayerInfo {
  std::string name;
  tcp::endpoint endpoint;
};

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

  friend std::ostream& operator<<(std::ostream& os, const Player& p) {
    os << "{" << p.name << " : " << p.address << "}";
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
  bool operator==(const Position& pos2) const {
    return (x == pos2.x && y == pos2.y);
  }
  bool operator<(const Position& pos2) const {
    if (x < pos2.x) {
      return true;
    } else if (x == pos2.x) {
      return y < pos2.y;
    }
    return false;
  }



  friend std::ostream& operator<<(std::ostream& os, const Position& p) {
    os << "{" << p.x << " : " << p.y << "}";
    return os;
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

  friend std::ostream& operator<<(std::ostream& os, Bomb& p) {
    os << "{" << p.position << " : " << p.timer << "}";
    return os;
  }
};

#endif  // SIK_ZAD3_UTILS_H
