//
// Created by Adam Al-Hosam on 05/05/2022.
//

#ifndef SIK_ZAD3_MESSAGES_H
#define SIK_ZAD3_MESSAGES_H

#include <map>
#include <memory>
#include <ostream>

#include "ByteStream.h"

class Message {
 public:
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

 public:
  static std::shared_ptr<DrawMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<DrawMessage>(
        std::make_shared<Lobby>(rest));
  }
  explicit Lobby(ByteStream& stream) {
    stream >> server_name >> players_count >> size_x >> size_y >> game_length >>
        explosion_radius >> bomb_timer;
  };
  void say_hello() override {
    std::cout << "I am Lobby";
  }
  friend std::ostream& operator<<(std::ostream& os, const Lobby& lobby) {
    os
       << " server_name: " << lobby.server_name
       << " players_count: " << lobby.players_count
       << " size_x: " << lobby.size_x << " size_y: " << lobby.size_y
       << " game_length: " << lobby.game_length
       << " explosion_radius: " << lobby.explosion_radius
       << " bomb_timer: " << lobby.bomb_timer;
    return os;
  }
};

#endif  // SIK_ZAD3_MESSAGES_H
