//
// Created by ciriubuntu on 17.05.22.
//

#ifndef SIK_ZAD3_CLIENTSERIALIZATION_H
#define SIK_ZAD3_CLIENTSERIALIZATION_H

#include <iostream>
#include "ByteStream.h"
#include "ClientState.h"
#include "utils.h"

class InvalidMessageException: public std::exception {
  [[nodiscard]] const char* what() const noexcept override {
    return "Invalid message read from buffer";
  }
};

class Message {
 public:
  virtual void say_hello() = 0;
};

class Sendable : public Message {
 public:
  virtual void serialize(ByteStream& os) = 0;
  virtual uint8_t getId() = 0;
};

class Join : public Sendable {
 private:
  std::string name;

 public:

  explicit Join(ByteStream& stream) {
    stream >> name;
  };
  explicit Join(std::string name) : name(std::move(name)){};

  void serialize(ByteStream& os) override {
    os << getId() << name;
  }

  void say_hello() override {
    std::cerr << "I Am Join"
              << " name: " << name;
  }

  uint8_t getId() override {
    return 0;
  }

};

class InputMessage : public Sendable {
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
    if (!input_message_map().contains(c)) {
      throw InvalidMessageException();
    }
    return input_message_map()[c](istr);
  }
};


class PlaceBomb : public InputMessage {
 private:
 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::make_shared<PlaceBomb>(rest);
  }
  explicit PlaceBomb(ByteStream& rest){};

  void serialize(ByteStream& os) override {
    os << getId();
  }

  void say_hello() override {
    std::cerr << "I am placebomb";
  }

  uint8_t getId() override {
    return 1;
  }

};


class PlaceBlock : public InputMessage {
 private:
 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::make_shared<PlaceBlock>(rest);
  }
  explicit PlaceBlock(ByteStream& rest){};

  void serialize(ByteStream& os) override {
    os << getId();
  }

  void say_hello() override {
    std::cerr << "I am placeblock";
  }

  uint8_t getId() override {
    return 2;
  }

};

class Move : public InputMessage {
 private:
  uint8_t direction;

 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return
        std::make_shared<Move>(rest);
  }
  explicit Move(ByteStream& rest) : direction(0) {
    rest >> direction;
  };

  void serialize(ByteStream& os) override {
    os << getId() << direction;
  }

  void say_hello() override {
    std::cerr << "I am Move in dir: " << direction << '\n';
  }

  uint8_t getId() override {
    return 3;
  }

};

// maybe add interface to game messages



class DrawMessage : public Sendable {};


class Game : public DrawMessage {
 private:
  std::string server_name;
  uint16_t size_x{};
  uint16_t size_y{};
  uint16_t game_length{};
  uint16_t turn{};
  std::map<PlayerId, Player> players;
  std::map<PlayerId, Position> player_positions;
  std::vector<Position> blocks;
  std::vector<Bomb> bombs;
  std::vector<Position> explosions;
  std::map<PlayerId, Score> scores;

 public:

  Game() = default;
  explicit Game(ByteStream& stream) {
    stream >> server_name >> size_x >> size_y >> game_length >> turn >>
        players >> player_positions >> blocks >> bombs >> explosions >> scores;
  };
  explicit Game(const ClientState& c) {
    server_name = c.server_name;
    size_x = c.size_x;
    size_y = c.size_y;
    game_length = c.game_length;
    turn = c.turn;
    players = c.players;
    player_positions = c.positions;
    blocks = std::vector<Position>(c.blocks.begin(), c.blocks.end());
    for (auto [key, val] : c.bombs) {
      bombs.push_back(val);
    }
    explosions = c.explosions;
    scores = c.scores;
  }

  void serialize(ByteStream& os) override {
    os << getId() << server_name << size_x << size_y << game_length << turn
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

  uint8_t getId() override {
    return 1;
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
  std::map<PlayerId, Player> players;

 public:

  Lobby() = default;
  explicit Lobby(const ClientState& c) {
    server_name = c.server_name;
    players_count = c.players_count;
    size_x = c.size_x;
    size_y = c.size_y;
    game_length = c.game_length;
    explosion_radius = c.explosion_radius;
    bomb_timer = c.bomb_timer;
    players = c.players;
  };
  explicit Lobby(ByteStream& stream) {
    stream >> server_name >> players_count >> size_x >> size_y >> game_length >>
        explosion_radius >> bomb_timer >> players;
  }

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

  uint8_t getId() override {
    return 0;
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

  virtual bool updateClientState(ClientState& state_to_upd) = 0;

  void say_hello() override {
    std::cerr << " serv message ";
  }

  static std::shared_ptr<ServerMessage> unserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    if (!server_message_map().contains(c)) {
      throw InvalidMessageException();
    }
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

  void say_hello() override {
    std::cerr << " server_name: " << server_name
              << " players_count: " << players_count << " size_x: " << size_x
              << " size_y: " << size_y << " game_length: " << game_length
              << " explosion_radius: " << explosion_radius
              << " bomb_timer: " << bomb_timer;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.server_name = server_name;
    state_to_upd.players_count = players_count;
    state_to_upd.size_x = size_x;
    state_to_upd.size_y = size_y;
    state_to_upd.game_length = game_length;
    state_to_upd.explosion_radius = explosion_radius;
    state_to_upd.bomb_timer = bomb_timer;

    return false;
  }
};

class AcceptedPlayer : public ServerMessage {
 private:
  PlayerId id{};
  Player player;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<AcceptedPlayer>(rest);
  }
  explicit AcceptedPlayer(ByteStream& stream) {
    stream >> id >> player;
  };

  void say_hello() override {
    std::cerr << "I am AcceptedPlayer "
              << " id: " << id << " player: " << player;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.players.insert({id, player});

    return true;
  }
};

class GameStarted : public ServerMessage {
 private:
  std::map<PlayerId, Player> players;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<GameStarted>(rest);
  }
  explicit GameStarted(ByteStream& stream) {
    stream >> players;
  };

  void say_hello() override {
    std::cerr << " players: " << players;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.game_on = true;
    state_to_upd.players = players;

    return false;
  }
};

class Event : public ServerMessage {
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
    return std::make_shared<BombPlaced>(rest);
  }
  explicit BombPlaced(ByteStream& stream) {
    stream >> id >> position;
  };

  explicit BombPlaced(BombId id, Position pos) : id(id), position(pos){};

  void say_hello() override {
    std::cerr << "I am BombPlaced id: " << id << " posiiton " << position;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.addBomb(id, position);

    return true;
  }
};



class BombExploded : public Event {
 private:
  BombId id{};
  std::vector<PlayerId> robots_destroyed;
  std::vector<Position> blocks_destroyed;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<BombExploded>(rest);
  }
  explicit BombExploded(ByteStream& stream) {
    stream >> id >> robots_destroyed >> blocks_destroyed;
  };

  void say_hello() override {
    std::cerr << "I am BombExploded id: " << id << "robots destroyed: \n "
              << robots_destroyed << " blocks destr\n"
              << blocks_destroyed;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.explosions.push_back(state_to_upd.bombs[id].position);
    state_to_upd.bombs.erase(id);
    for (auto& robotId : robots_destroyed) {
      state_to_upd.would_die.insert(robotId);
    }
    for (auto const& block : blocks_destroyed) {
      state_to_upd.blocks.erase(block);
    }

    return true;
  }
};

class PlayerMoved : public Event {
 private:
  PlayerId id{};
  Position position;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<PlayerMoved>(rest);
  }
  explicit PlayerMoved(ByteStream& stream) {
    stream >> id >> position;
  };

  explicit PlayerMoved(PlayerId id, Position pos) : id(id), position(pos) {};

  void say_hello() override {
    std::cerr << "I am PlayerMoved id: " << id << " posiiton " << position;
  }
  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.positions[id] = position;

    return true;
  }
};

class BlockPlaced : public Event {
 private:
  Position position;

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<BlockPlaced>(rest);
  }
  explicit BlockPlaced(ByteStream& stream) {
    stream >> position;
  };

  explicit BlockPlaced(Position pos) : position(pos) {};

  void say_hello() override {
    std::cerr << "I am BlockPlaced posiiton: " << position;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.blocks.insert(position);

    return true;
  }
};

class Turn : public ServerMessage {
 private:
  uint16_t turn{};
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
    for (auto& event : events) {
      event->say_hello();
      std::cerr << '\n';
    }
  }


  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.explosions.clear();
    state_to_upd.turn = turn;
    for (auto& event : events) {
      event->updateClientState(state_to_upd);
    }
    for (auto id : state_to_upd.would_die) {
      state_to_upd.scores[id]++;
    }
    state_to_upd.would_die.clear();
    return true;
  }
};

class GameEnded : public ServerMessage {
 private:
  std::map<PlayerId, Score> scores;

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<GameEnded>(rest);
  }
  explicit GameEnded(ByteStream& stream) {
    stream >> scores;
  };


  void say_hello() override {
    std::cerr << "I am GameEnded "
              << " scores: \n"
              << scores;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.scores = scores;
    state_to_upd.game_on = false;

    return false;
  }
};

void registerAll() {
  InputMessage::register_to_map(0, PlaceBomb::create);
  InputMessage::register_to_map(1, PlaceBlock::create);
  InputMessage::register_to_map(2, Move::create);
  Event::register_to_map(0, BombPlaced::create);
  Event::register_to_map(1, BombExploded::create);
  Event::register_to_map(2, PlayerMoved::create);
  Event::register_to_map(3, BlockPlaced::create);
  ServerMessage::register_to_map(0, Hello::create);
  ServerMessage::register_to_map(1, AcceptedPlayer::create);
  ServerMessage::register_to_map(2, GameStarted::create);
  ServerMessage::register_to_map(3, Turn::create);
  GameEnded::register_to_map(4, GameEnded::create);
}


#endif  // SIK_ZAD3_CLIENTSERIALIZATION_H
