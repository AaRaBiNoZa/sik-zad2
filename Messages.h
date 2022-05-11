//
// Created by Adam Al-Hosam on 05/05/2022.
//

#ifndef SIK_ZAD3_MESSAGES_H
#define SIK_ZAD3_MESSAGES_H

#include <map>
#include <memory>
#include <ostream>
#include <utility>

#include "ByteStream.h"

std::ostream& operator<<(std::ostream& os, uint8_t x) {
  os << (char) (x + '0');

  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> v) {
  for (auto k: v) {
    os << k << '\n';
  }

  return os;
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, std::map<T1, T2> v) {
  for (auto [k, val]: v) {
    os << "{" << k << " : " << val << "}" << '\n';
  }

  return os;
}

class Player {
 private:
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

  friend std::ostream& operator<<(std::ostream& os, Player& p) {
    os << "{" << p.name << " : " << p.address << "}";
    return os;
  }
};

class Position {
 private:
 public:
  uint16_t x{};
  uint16_t y{};
  std::string name;
  std::string address;
  Position(uint16_t x, uint16_t y) : x(x), y(y){};
  Position() : name(), address(){};
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

  friend std::ostream& operator<<(std::ostream& os, Position& p) {
    os << "{" << p.x << " : " << p.y << "}";
    return os;
  }
};

class Bomb {
 private:
 public:
  Position position;
  uint16_t timer;

  Bomb(Position pos, uint16_t timer) : position(std::move(pos)), timer(timer){};
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

  friend std::ostream& operator<<(std::ostream& os, Bomb& p) {
    os << "{" << p.position << " : " << p.timer << "}";
    return os;
  }
};

class Message {
 public:
  using PlayerId = uint8_t;
  using BombId = uint32_t;
  using Score = uint32_t;

  typedef std::shared_ptr<Message> (*MessageFactory)(ByteStream&);
  static std::map<char, MessageFactory>& message_map() {
    static auto* ans = new std::map<char, MessageFactory>();
    return *ans;
  }
  static void register_to_map(char key, MessageFactory val) {
    message_map().insert({key, val});
  }
  virtual void say_hello() {
    std::cout << "Hello i am Message";
  };
  static std::shared_ptr<Message> unserialize(ByteStream& istr) {
    char c;
    istr >> c;
    return message_map()[c](istr);
  }

  // validate len
  static std::string readString(ByteStream& istr) {
    uint8_t len;
    istr >> len;
    std::string res;
    res.resize(len);
    istr >> res;

    return res;
  }
};

class InputMessage : public Message {
 public:
  typedef std::shared_ptr<InputMessage> (*InputMessageFactory)(ByteStream&);
  static std::map<char, InputMessageFactory>& input_message_map() {
    static auto* ans = new std::map<char, InputMessageFactory>();
    return *ans;
  };
  static void register_to_map(char key, InputMessageFactory val) {
    input_message_map().insert({key, val});
  }

  static std::shared_ptr<Message> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<Message>(unserialize(rest));
  }

  static std::shared_ptr<InputMessage> unserialize(ByteStream& istr) {
    char c;
    istr >> c;
    return input_message_map()[c](istr);
  }
};

class PlaceBomb : public InputMessage {
 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<InputMessage>(
        std::make_shared<PlaceBomb>(rest));
  }
  explicit PlaceBomb(ByteStream& rest){};
  void say_hello() override {
    std::cout << "I am placebomb";
  }
};

class PlaceBlock : public InputMessage {
 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<InputMessage>(
        std::make_shared<PlaceBlock>(rest));
  }
  explicit PlaceBlock(ByteStream& rest){};
  void say_hello() override {
    std::cout << "I am placeblock";
  }
};

class Move : public InputMessage {
 private:
  uint8_t direction;

 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<InputMessage>(
        std::make_shared<Move>(rest));
  }
  explicit Move(ByteStream& rest) : direction(0) {
    rest >> direction;
  };
  void say_hello() override {
    std::cout << "I am placeblock";
  }
};

class DrawMessage : public Message {
 public:
  typedef std::shared_ptr<DrawMessage> (*DrawMessageFactory)(ByteStream&);
  static std::map<char, DrawMessageFactory>& draw_message_map() {
    static auto* ans = new std::map<char, DrawMessageFactory>();
    return *ans;
  };
  static void register_to_map(char key, DrawMessageFactory val) {
    draw_message_map().insert({key, val});
  }

  static std::shared_ptr<Message> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<Message>(unserialize(rest));
  }

  static std::shared_ptr<DrawMessage> unserialize(ByteStream& istr) {
    char c;
    istr >> c;
    return draw_message_map()[c](istr);
  }
};

class Lobby : public DrawMessage {
 private:
  std::string server_name;
  uint8_t players_count{};
  uint16_t size_x{};
  uint16_t size_y{};
  uint16_t game_length{};
  uint16_t explosion_radius{};
  uint16_t bomb_timer{};
  std::map<Message::PlayerId, Player> players;

 public:
  static std::shared_ptr<DrawMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<DrawMessage>(
        std::make_shared<Lobby>(rest));
  }
  explicit Lobby(ByteStream& stream) {
    stream >> server_name >> players_count >> size_x >> size_y >> game_length >>
        explosion_radius >> bomb_timer >> players;
  };
  void say_hello() override {
    std::cout
       << " server_name: " << this->server_name
       << " players_count: " << this->players_count
       << " size_x: " << this->size_x << " size_y: " << this->size_y
       << " game_length: " << this->game_length
       << " explosion_radius: " << this->explosion_radius
       << " bomb_timer: " << this->bomb_timer << " players: " << this->players;
  }

};

class Game : public DrawMessage {
 private:
  std::string server_name;
  uint16_t size_x{};
  uint16_t size_y{};
  uint16_t game_length{};
  uint16_t turn{};
  std::map<Message::PlayerId, Player> players;
  std::map<PlayerId, Position> player_positions;
  std::vector<Position> blocks;
  std::vector<Bomb> bombs;
  std::vector<Position> explosions;
  std::map<PlayerId, Score> scores;

 public:
  static std::shared_ptr<DrawMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<DrawMessage>(std::make_shared<Game>(rest));
  }
  explicit Game(ByteStream& stream) {
    stream >> server_name >> size_x >> size_y >> game_length >> turn >>
        players >> player_positions >> blocks >> bombs >> explosions >> scores;
  };
  void say_hello() override {
    std::cout
       << " server_name: " << this->server_name << " size_x: " << this->size_x
       << " size_y: " << this->size_y << " game_length: " << this->game_length
       << " turn: " << this->turn << " players: " << this->players
       << " player_positions: " << this->player_positions
       << " blocks: " << this->blocks << " bombs: " << this->bombs
       << " explosions: " << this->explosions << " scores: " << this->scores;
  }
};

class ClientMessage : public Message {
 public:
  typedef std::shared_ptr<ClientMessage> (*ClientMessageFactory)(ByteStream&);
  static std::map<char, ClientMessageFactory>& client_message_map() {
    static auto* ans = new std::map<char, ClientMessageFactory>();
    return *ans;
  };
  static void register_to_map(char key, ClientMessageFactory val) {
    client_message_map().insert({key, val});
  }

  static std::shared_ptr<Message> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<Message>(unserialize(rest));
  }

  static std::shared_ptr<ClientMessage> unserialize(ByteStream& istr) {
    char c;
    istr >> c;
    return client_message_map()[c](istr);
  }
};

class Join : public ClientMessage {
 private:
  std::string name;

 public:
  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<ClientMessage>(std::make_shared<Join>(rest));
  }
  explicit Join(ByteStream& stream) {
    stream >> name;
  };
  void say_hello() override {
    std::cout << "I Am Join" << " name: " << name;
  }
};

#endif  // SIK_ZAD3_MESSAGES_H
