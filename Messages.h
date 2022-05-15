//
// Created by Adam Al-Hosam on 05/05/2022.
//

#ifndef SIK_ZAD3_MESSAGES_H
#define SIK_ZAD3_MESSAGES_H

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "ByteStream.h"
#include "ClientState.h"
#include "Randomizer.h"
#include "ServerState.h"
#include "common.h"
#include "utils.h"

class Message {
 public:
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
  std::map<PlayerId, Player> players;

 public:
  static std::shared_ptr<DrawMessage> create(ByteStream& rest) {
    return std::make_shared<Lobby>(rest);
  }
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
};

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
  static std::shared_ptr<DrawMessage> create(ByteStream& rest) {
    return std::make_shared<Game>(rest);
  }
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
    os << (uint8_t)0 << server_name << players_count << size_x << size_y
       << game_length << explosion_radius << bomb_timer;
  }
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

  void serialize(ByteStream& os) override {
    os << (uint8_t)1 << id << player;
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

  void serialize(ByteStream& os) override {
    os << (uint8_t)2 << players;
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.game_on = true;
    state_to_upd.players = players;

    return false;
  }
};

class Event : public ServerMessage {  // moze dziedziczyc po turn
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

  void serialize(ByteStream& os) override {
    os << (uint8_t)0 << id << position;
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

  void serialize(ByteStream& os) override {
    os << (uint8_t)1 << id << robots_destroyed << blocks_destroyed;
  }

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

  void serialize(ByteStream& os) override {
    os << (uint8_t)2 << id << position;
  }

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

  void serialize(ByteStream& os) override {
    os << (uint8_t)3 << position;
  }
  bool updateClientState(ClientState& state_to_upd) override {
        state_to_upd.blocks.insert(position);

    return true;
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
    for (auto& event : events) {
      event->say_hello();
      std::cerr << '\n';
    }
  }

  void serialize(ByteStream& os) override {
    os << (uint8_t)3;
    for (auto& event : events) {
      event->serialize(os);
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

  void serialize(ByteStream& os) override {
    os << (uint8_t)3 << scores;
  }

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

  virtual void updateServerState(ServerState& state_to_upd,
                                 std::optional<PlayerId>& player_id) = 0;
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

class PlaceBlock : public InputMessage, public ClientMessage {
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

  void updateServerState(ServerState& state_to_upd,
                         std::optional<PlayerId> &player_id) override {
    if (player_id) {
      state_to_upd.addBlock(player_id.value());
      state_to_upd.addEvent(std::make_shared<BlockPlaced>(state_to_upd.getPlayerPosition(player_id.value())));
    }

  }
};

class Move : public InputMessage, public ClientMessage {
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
    std::cerr << "I am Move in dir: " << direction << '\n';
  }
  void serialize(ByteStream& os) override {
    os << (uint8_t)3 << direction;
  }

  void updateServerState(ServerState& state_to_upd,
                         std::optional<PlayerId> &player_id) override {
    if (player_id) {
      Position new_pos = state_to_upd.move(direction, player_id.value());
      state_to_upd.addEvent(std::make_shared<PlayerMoved>(player_id.value(), new_pos));
    }

  }
};

// maybe add interface to game messages
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
  explicit Join(std::string name) : name(std::move(name)){};
  void serialize(ByteStream& os) override {
    os << (uint8_t)0 << name;
  }

  void say_hello() override {
    std::cerr << "I Am Join"
              << " name: " << name;
  }

  void updateServerState(ServerState& state_to_upd,
                         std::optional<PlayerId>& player_id) override {
    if (!player_id) {
      uint8_t possible_id = state_to_upd.next_player_id++;
      if (possible_id < state_to_upd.getMaxCount()) {
        player_id.emplace(possible_id);
      } else {
        state_to_upd.next_player_id--;
      }
    }
  }
};

class PlaceBomb : public InputMessage, public ClientMessage {
 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::make_shared<PlaceBomb>(rest);
  }
  explicit PlaceBomb(ByteStream& rest){};
  void say_hello() override {
    std::cerr << "I am placebomb";
  }
  void serialize(ByteStream& os) override {
    os << (uint8_t)1;
  }


  void updateServerState(ServerState& state_to_upd,
                         std::optional<PlayerId>& player_id) override {
    if (player_id) {
      BombId bomb_id = state_to_upd.game_state->addBomb(player_id.value(), state_to_upd.bomb_timer);
      Position player_pos = state_to_upd.getPlayerPosition(player_id.value());
      state_to_upd.addEvent(std::make_shared<BombPlaced>(bomb_id, player_pos));
    }

  }
  // caller should have exclusive access to state_to_upd
};

#endif  // SIK_ZAD3_MESSAGES_H
