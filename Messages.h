//
// Created by Adam Al-Hosam on 05/05/2022.
//

#ifndef SIK_ZAD3_MESSAGES_H
#define SIK_ZAD3_MESSAGES_H

#include <iostream>
#include <map>
#include <memory>
#include <utility>

#include "ByteStream.h"


std::ostream& operator<<(std::ostream& os, uint8_t x) {
  os << (char)(x + '0');

  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> v) {
  for (auto k : v) {
    os << k << '\n';
  }

  return os;
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, std::map<T1, T2> v) {
  for (auto [k, val] : v) {
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
  static std::map<uint8_t, MessageFactory>& message_map() {
    static auto* ans = new std::map<uint8_t, MessageFactory>();
    return *ans;
  }
  static void register_to_map(uint8_t key, MessageFactory val) {
    message_map().insert({key, val});
  }
  virtual void say_hello() {
    std::cerr << "Hello i am Message";
  };
  virtual void serialize(ByteStream& ostr) = 0;
  static std::shared_ptr<Message> unserialize(ByteStream& istr) {
    uint8_t c;
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

struct ClientState {
  std::string server_name;
  uint8_t players_count;
  uint16_t size_x;
  uint16_t size_y;
  uint16_t game_length;
  uint16_t explosion_radius;
  uint16_t bomb_timer;
  std::map<Message::PlayerId, Player> players;
  uint16_t turn;
  std::map<Message::PlayerId, Position> positions;
  std::vector<Position> blocks;
  std::vector<Bomb> bombs;
  std::vector<Position> explosions;
  std::map<Message::PlayerId, Message::Score> scores;
  bool game_on = false;
  bool am_i_playing = false;
};


class InputMessage : public Message {
 public:
  typedef std::shared_ptr<InputMessage> (*InputMessageFactory)(ByteStream&);
  static std::map<uint8_t, InputMessageFactory>& input_message_map() {
    static auto* ans = new std::map<uint8_t, InputMessageFactory>();
    return *ans;
  };
  static void register_to_map(uint8_t key, InputMessageFactory val) {
    input_message_map().insert({key, val});
  }

  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return unserialize(rest);
  }

  static std::shared_ptr<InputMessage> unserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    return input_message_map()[c](istr);
  }
};

class PlaceBomb : public InputMessage {
 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return
        std::make_shared<PlaceBomb>(rest);
  }
  explicit PlaceBomb(ByteStream& rest){};
  void say_hello() override {
    std::cerr << "I am placebomb";
  }
  void serialize(ByteStream& os) override {
    os << (uint8_t)1;
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
    std::cerr << "I am placeblock";
  }
  void serialize(ByteStream& os) override {
    os << (uint8_t)2;
  }
};

class Move : public InputMessage {
 private:
  uint8_t direction;

 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::dynamic_pointer_cast<InputMessage>(std::make_shared<Move>(rest));
  }
  explicit Move(ByteStream& rest) : direction(0) {
    rest >> direction;
  };
  void say_hello() override {
    std::cerr << "I am Move in dir: " << direction << '\n';
  }
  void serialize(ByteStream& os) override {
    os << (uint8_t)3 << direction;
  }
};

class DrawMessage : public Message {
 public:
  typedef std::shared_ptr<DrawMessage> (*DrawMessageFactory)(ByteStream&);

  static std::map<uint8_t, DrawMessageFactory>& draw_message_map() {
    static auto* ans = new std::map<uint8_t, DrawMessageFactory>();
    return *ans;
  };
  static void register_to_map(uint8_t key, DrawMessageFactory val) {
    draw_message_map().insert({key, val});
  }

  static std::shared_ptr<DrawMessage> create(ByteStream& rest) {
    return unserialize(rest);
  }

  static std::shared_ptr<DrawMessage> unserialize(ByteStream& istr) {
    uint8_t c;
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
    return std::make_shared<Lobby>(rest);
  }
  Lobby() = default;
  explicit Lobby(ByteStream& stream) {
    stream >> server_name >> players_count >> size_x >> size_y >> game_length >>
        explosion_radius >> bomb_timer >> players;
  };

  void serialize(ByteStream& os) override {
    os << (uint8_t)0 << server_name << players_count << size_x << size_y
       << game_length << explosion_radius << bomb_timer << players;
  }

  void say_hello() override {
    std::cerr << " server_name: " << this->server_name
              << " players_count: " << this->players_count
              << " size_x: " << this->size_x << " size_y: " << this->size_y
              << " game_length: " << this->game_length
              << " explosion_radius: " << this->explosion_radius
              << " bomb_timer: " << this->bomb_timer
              << " players: " << this->players;
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
    return std::make_shared<Game>(rest);
  }
  Game() = default;
  explicit Game(ByteStream& stream) {
    stream >> server_name >> size_x >> size_y >> game_length >> turn >>
        players >> player_positions >> blocks >> bombs >> explosions >> scores;
  };

  void serialize(ByteStream& os) override {
    os << (uint8_t)1 << server_name << size_x << size_y << game_length << turn
       << players << player_positions << blocks << bombs << explosions
       << scores;
  }

  void say_hello() override {
    std::cerr << " server_name: " << this->server_name
              << " size_x: " << this->size_x << " size_y: " << this->size_y
              << " game_length: " << this->game_length
              << " turn: " << this->turn << " players: " << this->players
              << " player_positions: " << this->player_positions
              << " blocks: " << this->blocks << " bombs: " << this->bombs
              << " explosions: " << this->explosions
              << " scores: " << this->scores;
  }
};

class ClientMessage : public Message {
 public:
  typedef std::shared_ptr<ClientMessage> (*ClientMessageFactory)(ByteStream&);
  static std::map<uint8_t, ClientMessageFactory>& client_message_map() {
    static auto* ans = new std::map<uint8_t, ClientMessageFactory>();
    return *ans;
  };
  static void register_to_map(uint8_t key, ClientMessageFactory val) {
    client_message_map().insert({key, val});
  }

  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return unserialize(rest);
  }

  static std::shared_ptr<ClientMessage> unserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    return client_message_map()[c](istr);
  }
};

class Join : public ClientMessage {
 private:
  std::string name;

 public:
  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return std::make_shared<Join>(rest);
  }
  explicit Join(ByteStream& stream) {
    stream >> name;
  };
  explicit Join(std::string name) : name(std::move(name)){
  };
  void serialize(ByteStream& os) override {
    os << (uint8_t) 0 << name;
  }

  void say_hello() override {
    std::cerr << "I Am Join"
              << " name: " << name;
  }
};

class ServerMessage : public Message {
 public:
  typedef std::shared_ptr<ServerMessage> (*ServerMessageFactory)(ByteStream&);

  static std::map<uint8_t, ServerMessageFactory>& server_message_map() {
    static auto* ans = new std::map<uint8_t, ServerMessageFactory>();
    return *ans;
  };
  static void register_to_map(uint8_t key, ServerMessageFactory val) {
    server_message_map().insert({key, val});
  }

  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return unserialize(rest);
  }


  virtual void updateClientState(ClientState& state_to_upd) = 0;

  void serialize(ByteStream& os) override {
    return;
  }

  void say_hello() override {
    std::cerr << " serv message ";
  }

  static std::shared_ptr<ServerMessage> unserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    return server_message_map()[c](istr);
  }
};

class Hello : public ServerMessage {
 private:
  std::string server_name;
  uint8_t players_count{};
  uint16_t size_x{};
  uint16_t size_y{};
  uint16_t game_length{};
  uint16_t explosion_radius{};
  uint16_t bomb_timer{};

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<Hello>(rest);
  }
  explicit Hello(ByteStream& stream) {
    stream >> server_name >> players_count >> size_x >> size_y >> game_length >>
        explosion_radius >> bomb_timer;
  };
  void serialize(ByteStream& os) override {
    os << (uint8_t) 0 << server_name << players_count << size_x << size_y << game_length << explosion_radius << bomb_timer;
  }
  void say_hello() override {
    std::cerr << " server_name: " << server_name
              << " players_count: " << players_count << " size_x: " << size_x
              << " size_y: " << size_y << " game_length: " << game_length
              << " explosion_radius: " << explosion_radius
              << " bomb_timer: " << bomb_timer;
  }

  void updateClientState(ClientState& state_to_upd) override {
    state_to_upd.server_name = server_name;
    state_to_upd.players_count = players_count;
    state_to_upd.size_x = size_x;
    state_to_upd.size_y = size_y;
    state_to_upd.game_length = game_length;
    state_to_upd.explosion_radius = explosion_radius;
    state_to_upd.bomb_timer = bomb_timer;
  }
};

class AcceptedPlayer : public ServerMessage {
 private:
  PlayerId id{};
  Player player;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return
        std::make_shared<AcceptedPlayer>(rest);
  }
  explicit AcceptedPlayer(ByteStream& stream) {
    stream >> id >> player;
  };

  void say_hello() override {
    std::cerr << "I am AcceptedPlayer "
              << " id: " << id << " player: " << player;
  }

  void serialize(ByteStream& os) override {
    os << (uint8_t) 1 << id << player;
  }

  void updateClientState(ClientState& state_to_upd) override {
    state_to_upd.players.insert({id, player});
  }

};

class GameStarted : public ServerMessage {
 private:
  std::map<PlayerId, Player> players;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return
        std::make_shared<GameStarted>(rest);
  }
  explicit GameStarted(ByteStream& stream) {
    stream >> players;
  };

  void say_hello() override {
    std::cerr << " players: " << players;
  }

  void serialize(ByteStream& os) override {
    os << (uint8_t) 2 << players;
  }

  void updateClientState(ClientState& state_to_upd) override {
    state_to_upd.players = players;
  }
};

class Event : public ServerMessage { // moze dziedziczyc po turn
 public:
  typedef std::shared_ptr<Event> (*EventMessageFactory)(ByteStream&);


  static std::map<uint8_t, EventMessageFactory>& event_message_map() {
    static auto* ans = new std::map<uint8_t, EventMessageFactory>();
    return *ans;
  };
  static void register_to_map(uint8_t key, EventMessageFactory val) {
    event_message_map().insert({key, val});
  }

  static std::shared_ptr<Event> create(ByteStream& rest) {
    return unserialize(rest);
  }

  static std::shared_ptr<Event> unserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    return event_message_map()[c](istr);
  }
};

class BombPlaced : public Event {
 private:
  BombId id{};
  Position position;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return
        std::make_shared<BombPlaced>(rest);
  }
  explicit BombPlaced(ByteStream& stream) {
    stream >> id >> position;
  };

  void say_hello() override {
    std::cerr << "I am BombPlaced id: " << id << " posiiton " << position;
  }

  void serialize(ByteStream& os) override {
    os << (uint8_t) 0 << id << position;
  }


};

class BombExploded : public Event {
 private:
  BombId id{};
  std::vector<PlayerId> robots_destroyed;
  std::vector<Position> blocks_destroyed;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return
        std::make_shared<BombExploded>(rest);
  }
  explicit BombExploded(ByteStream& stream) {
    stream >> id >> robots_destroyed >> blocks_destroyed;
  };

  void serialize(ByteStream& os) override {
    os << (uint8_t) 1 << id << robots_destroyed << blocks_destroyed;
  }

  void say_hello() override {
    std::cerr << "I am BombExploded id: " << id << "robots destroyed: \n "
              << robots_destroyed << " blocks destr\n"
              << blocks_destroyed;
  }
};

class PlayerMoved : public Event {
 private:
  PlayerId id{};
  Position position;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return
        std::make_shared<PlayerMoved>(rest);
  }
  explicit PlayerMoved(ByteStream& stream) {
    stream >> id >> position;
  };

  void serialize(ByteStream& os) override {
    os << (uint8_t) 2 << id << position;
  }

  void say_hello() override {
    std::cerr << "I am PlayerMoved id: " << id << " posiiton " << position;
  }
};

class BlockPlaced : public Event {
 private:
  Position position;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return
        std::make_shared<BlockPlaced>(rest);
  }
  explicit BlockPlaced(ByteStream& stream) {
    stream >> position;
  };

  void say_hello() override {
    std::cerr << "I am BlockPlaced posiiton: " << position;
  }

  void serialize(ByteStream& os) override {
    os << (uint8_t) 3 << position;
  }

};

class Turn : public ServerMessage {
 private:
  uint16_t turn;
  std::vector<std::shared_ptr<Event>> events;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<Turn>(rest);
  }
  explicit Turn(ByteStream& stream) {
    stream >> turn;
    uint32_t len;
    stream >> len;
    events.resize(len);
    for (int i = 0; i < len; ++i) {
      events[i] = Event::create(stream);
    }
  };

  void say_hello() override {
    std::cerr << " I AM TURN ";
  }

  void serialize(ByteStream& os) override {
    os << (uint8_t) 3;
    for (auto& event: events) {
      event->serialize(os);
    }
  }

  void updateClientState(ClientState& state_to_upd) override {
    state_to_upd.turn = turn;
    for (auto& event: events) {

    }
  }
};

class GameEnded : public ServerMessage {
 private:
  std::map<PlayerId, Score> scores;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return
        std::make_shared<GameEnded>(rest);
  }
  explicit GameEnded(ByteStream& stream) {
    stream >> scores;
  };

  void serialize(ByteStream& os) override {
    os << (uint8_t) 3 << scores;
  }

  void say_hello() override {
    std::cerr << "I am GameEnded "
              << " scores: \n"
              << scores;
  }
};

#endif  // SIK_ZAD3_MESSAGES_H
