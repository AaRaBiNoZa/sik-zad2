#ifndef SIK_ZAD3_CLIENTSERIALIZATION_H
#define SIK_ZAD3_CLIENTSERIALIZATION_H

#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <vector>

#include "ByteStream.h"
#include "ClientState.h"
#include "ServerState.h"
#include "utils.h"

/**
 * This is a huge file defining all possible interfaces and
 * message types (from client's point of view)
 */

/**
 * If a function pointer with given id can't be found by a factory.
 * (when some submessage has a wrong id)
 */
class InvalidMessageException : public std::exception {
  [[nodiscard]] const char* what() const noexcept override {
    return "Invalid message read from buffer";
  }
};

/**
 * May be needed in the future for setting an interface.
 */
class Message {
 public:
  virtual ~Message() = default;
};

/**
 * Represents a message type that should be able to be serialized.
 * getId() is supposed to return a messages id used to serialize.
 */
class Sendable : public Message {
 private:
  virtual uint8_t getId() = 0;

 public:
  virtual void serialize(ByteStream& os) = 0;
  virtual ~Sendable() = default;
};

class Join : public Sendable {
 private:
  std::string name;

  uint8_t getId() override {
    return 0;
  }

 public:
  explicit Join(std::string name) : name(std::move(name)){};

  void serialize(ByteStream& os) override {
    os << getId() << name;
  }
};

/**
 * This is a factory that is supposed to be able to deserialize messages
 * of type InputMessage
 */
class InputMessage : public Sendable {
 public:
  typedef std::shared_ptr<InputMessage> (*InputMessageFactory)(ByteStream&);
  static std::map<uint8_t, InputMessageFactory>& input_message_map() {
    static auto ans = new std::map<uint8_t, InputMessageFactory>();
    return *ans;
  };

  /**
   * used to populate function ptr map
   */
  static void register_to_map(uint8_t key, InputMessageFactory val) {
    input_message_map().insert({key, val});
  }

  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return deserialize(rest);
  }

  static std::shared_ptr<InputMessage> deserialize(ByteStream& istr) {
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
  uint8_t getId() override {
    return 1;
  }

 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::make_shared<PlaceBomb>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit PlaceBomb([[maybe_unused]] ByteStream& rest){};

  void serialize(ByteStream& os) override {
    os << getId();
  }
};

class PlaceBlock : public InputMessage {
 private:
  uint8_t getId() override {
    return 2;
  }

 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::make_shared<PlaceBlock>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit PlaceBlock([[maybe_unused]] ByteStream& rest){};

  void serialize(ByteStream& os) override {
    os << getId();
  }
};

class Move : public InputMessage {
 private:
  uint8_t direction;

  uint8_t getId() override {
    return 3;
  }

 public:
  static std::shared_ptr<InputMessage> create(ByteStream& rest) {
    return std::make_shared<Move>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit Move(ByteStream& rest) : direction(0) {
    rest >> direction;
  };

  void serialize(ByteStream& os) override {
    os << getId() << direction;
  }
};

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
  std::set<Position> blocks;
  std::vector<Bomb> bombs;
  std::set<Position> explosions;
  std::map<PlayerId, Score> scores;

  uint8_t getId() override {
    return 1;
  }

 public:
  Game() = default;

  explicit Game(const ClientState& c) {
    server_name = c.server_name;
    size_x = c.size_x;
    size_y = c.size_y;
    game_length = c.game_length;
    turn = c.turn;
    players = c.players;
    player_positions = c.positions;
    blocks = c.blocks;
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

  uint8_t getId() override {
    return 0;
  }

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

  void serialize(ByteStream& os) override {
    os << (uint8_t)0 << server_name << players_count << size_x << size_y
       << game_length << explosion_radius << bomb_timer << players;
  }
};

class ServerMessage : public Sendable {
 public:
  typedef std::shared_ptr<ServerMessage> (*ServerMessageFactory)(ByteStream&);

  static std::map<uint8_t, ServerMessageFactory>& server_message_map() {
    static auto* ans = new std::map<uint8_t, ServerMessageFactory>();
    return *ans;
  };

  /**
   * used to populate function ptr map
   */
  static void register_to_map(uint8_t key, ServerMessageFactory val) {
    server_message_map().insert({key, val});
  }

  /**
   * True means that state has changed (we should send message to GUI)
   */
  virtual bool updateClientState(ClientState& state_to_upd) = 0;

  static std::shared_ptr<ServerMessage> deserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    if (!server_message_map().contains(c)) {
      throw InvalidMessageException();
    }
    return server_message_map()[c](istr);
  }

  virtual ~ServerMessage() = default;
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

  uint8_t getId() override {
    return 0;
  }

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<Hello>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit Hello(ByteStream& stream) {
    stream >> server_name >> players_count >> size_x >> size_y >> game_length >>
        explosion_radius >> bomb_timer;
  };

  explicit Hello(ServerState& state) {
    server_name = state.getServerName();
    players_count = state.getPlayersCount();
    size_x = state.getSizeX();
    size_y = state.getSizeY();
    game_length = state.getGameLength();
    explosion_radius = state.getExplosionRadius();
    bomb_timer = state.getBombTimer();
  };

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.server_name = server_name;
    state_to_upd.players_count = players_count;
    state_to_upd.size_x = size_x;
    state_to_upd.size_y = size_y;
    state_to_upd.game_length = game_length;
    state_to_upd.explosion_radius = explosion_radius;
    state_to_upd.bomb_timer = bomb_timer;

    return true;
  }

  void serialize(ByteStream& os) override {
    os << getId() << server_name << players_count << size_x << size_y
       << game_length << explosion_radius << bomb_timer;
  }
};

class AcceptedPlayer : public ServerMessage {
 private:
  PlayerId id{};
  Player player;

  uint8_t getId() override {
    return 1;
  }

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<AcceptedPlayer>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit AcceptedPlayer(ByteStream& stream) {
    stream >> id >> player;
  };

  AcceptedPlayer(PlayerId id, Player player) : id(id), player(player){};

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.players.insert({id, player});
    state_to_upd.scores.insert({id, 0});

    return true;
  }

  void serialize(ByteStream& os) override {
    os << getId() << id << player;
  }
};

class GameStarted : public ServerMessage {
 private:
  std::map<PlayerId, Player> players;

  uint8_t getId() override {
    return 2;
  }

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<GameStarted>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit GameStarted(ByteStream& stream) {
    stream >> players;
  };

  explicit GameStarted(ServerState& state) {
    players = state.getPlayers();
  };

  explicit GameStarted(std::map<PlayerId, Player> players_in) {
    players = players_in;
  };

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.game_on = true;
    state_to_upd.players = players;
    for (auto& k : players) {
      state_to_upd.scores[k.first] = 0;
    }

    return false;
  }

  void serialize(ByteStream& os) override {
    os << getId() << players;
  }
};

class Event : public ServerMessage {
 public:
  typedef std::shared_ptr<Event> (*EventMessageFactory)(ByteStream&);

  static std::map<uint8_t, EventMessageFactory>& event_message_map() {
    static auto* ans = new std::map<uint8_t, EventMessageFactory>();
    return *ans;
  };

  /**
   * used to populate function ptr map
   */
  static void register_to_map(uint8_t key, EventMessageFactory val) {
    event_message_map().insert({key, val});
  }

  static std::shared_ptr<Event> create(ByteStream& rest) {
    return deserialize(rest);
  }

  static std::shared_ptr<Event> deserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    return event_message_map()[c](istr);
  }
};

class BombPlaced : public Event {
 private:
  BombId id{};
  Position position;

  uint8_t getId() override {
    return 0;
  }

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<BombPlaced>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit BombPlaced(ByteStream& stream) {
    stream >> id >> position;
  };

  explicit BombPlaced(BombId id, Position pos) : id(id), position(pos){};

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.addBomb(id, position);

    return true;
  }

  void serialize(ByteStream& os) override {
    os << getId() << id << position;
  }
};

class BombExploded : public Event {
 private:
  BombId id{};
  std::vector<PlayerId> robots_destroyed;
  std::vector<Position> blocks_destroyed;

  uint8_t getId() override {
    return 1;
  }

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<BombExploded>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit BombExploded(ByteStream& stream) {
    stream >> id >> robots_destroyed >> blocks_destroyed;
  };

  BombExploded(BombId b_id, std::set<PlayerId> robots, std::set<Position> positions)
      : id(b_id), robots_destroyed(robots.begin(), robots.end()),
        blocks_destroyed(positions.begin(), positions.end()) {
  }

  BombExploded(){};


  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.calculateExplosions(state_to_upd.bombs[id].position);
    state_to_upd.bombs.erase(id);
    for (auto& robotId : robots_destroyed) {
      state_to_upd.would_die.insert(robotId);
    }
    for (auto const& block : blocks_destroyed) {
      state_to_upd.blocks_to_destroy.insert(block);
    }

    return true;
  }

  void serialize(ByteStream& os) override {
    os << getId() << id << robots_destroyed << blocks_destroyed;
  }
};

class PlayerMoved : public Event {
 private:
  PlayerId id{};
  Position position;

  uint8_t getId() override {
    return 2;
  }

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<PlayerMoved>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit PlayerMoved(ByteStream& stream) {
    stream >> id >> position;
  };

  explicit PlayerMoved(PlayerId id, Position pos) : id(id), position(pos){};

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.positions[id] = position;

    return true;
  }

  //  bool updateServerState(ServerState& state_to_upd)  {
  //    state_to_upd.setPlayerPos(id, position);
  //  }

  void serialize(ByteStream& os) override {
    os << getId() << id << position;
  }
};

class BlockPlaced : public Event {
 private:
  Position position;

  uint8_t getId() override {
    return 3;
  }

 public:
  static std::shared_ptr<Event> create(ByteStream& rest) {
    return std::make_shared<BlockPlaced>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit BlockPlaced(ByteStream& stream) {
    stream >> position;
  };

  explicit BlockPlaced(Position pos) : position(pos){};

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.blocks.insert(position);

    return true;
  }
  //  bool updateServerState(ServerState& state_to_upd)  {
  //    state_to_upd.addBlock(position);
  //  }

  void serialize(ByteStream& os) override {
    os << getId() << position;
  }
};

class Turn : public ServerMessage {
 private:
  uint16_t turn{};
  std::vector<std::shared_ptr<Event>> events;
  uint8_t getId() override {
    return 3;
  }

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<Turn>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit Turn(ByteStream& stream) {
    stream >> turn;
    uint32_t len;
    stream >> len;
    events.resize(len);
    for (size_t i = 0; i < len; ++i) {
      events[i] = Event::create(stream);
    }
  };

  Turn(uint16_t turn_id) : turn(turn_id){};

  void addEvent(std::shared_ptr<Event> ev) {
    events.push_back(ev);
  }

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.explosions.clear();
    state_to_upd.blocks_to_destroy.clear();
    state_to_upd.would_die.clear();

    state_to_upd.turn = turn;
    for (auto& [id, bomb] : state_to_upd.bombs) {
      bomb.timer--;
    }
    for (auto& event : events) {
      event->updateClientState(state_to_upd);
    }
    for (auto id : state_to_upd.would_die) {
      state_to_upd.scores[id]++;
    }
    for (auto destroyed : state_to_upd.blocks_to_destroy) {
      state_to_upd.blocks.erase(destroyed);
    }
    return true;
  }

  void serialize(ByteStream& os) override {
    os << getId() << turn << (uint32_t)events.size();
    for (const auto& event : events) {
      event->serialize(os);
    }
  }
};

class GameEnded : public ServerMessage {
 private:
  std::map<PlayerId, Score> scores;

  uint8_t getId() override {
    return 4;
  }

 public:
  static std::shared_ptr<ServerMessage> create(ByteStream& rest) {
    return std::make_shared<GameEnded>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit GameEnded(ByteStream& stream) {
    stream >> scores;
  };

  explicit GameEnded(std::map<PlayerId, Score> scores) {
    scores = scores;
  };

  bool updateClientState(ClientState& state_to_upd) override {
    state_to_upd.reset();

    return true;
  }

  void serialize(ByteStream& os) override {
    os << getId() << scores;
  }
};

/* May be changed so it is sendable instead of input message */
class ClientMessage : public Message {
 public:
  typedef std::shared_ptr<ClientMessage> (*ClientMessageFactory)(ByteStream&);

  static std::map<uint8_t, ClientMessageFactory>& client_message_map() {
    static auto* ans = new std::map<uint8_t, ClientMessageFactory>();
    return *ans;
  };

  /**
   * used to populate function ptr map
   */
  static void register_to_map(uint8_t key, ClientMessageFactory val) {
    client_message_map().insert({key, val});
  }

  virtual void updateServerState(ServerState& state_to_upd, PlayerId id,
                                 std::shared_ptr<Turn> cur_turn) = 0;
  virtual bool TryJoin([[maybe_unused]] ServerState& state_to_upd,
                       [[maybe_unused]] std::optional<PlayerId>& player_id,
                       std::string address) {
    return false;
  }

  static std::shared_ptr<ClientMessage> deserialize(ByteStream& istr) {
    uint8_t c;
    istr >> c;
    if (!client_message_map().contains(c)) {
      throw InvalidMessageException();
    }
    return client_message_map()[c](istr);
  }

  virtual bool amIJoin() {
    return false;
  }

  virtual ~ClientMessage() = default;
};
/* **************************************** */
class ClientJoin : public ClientMessage {
 private:
  std::string name;

 public:
  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return std::make_shared<ClientJoin>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit ClientJoin(ByteStream& stream) {
    stream >> name;
  };

  void updateServerState(ServerState& state_to_upd, PlayerId id,
                         std::shared_ptr<Turn> cur_turn) override {
    return;
  }

  bool TryJoin(ServerState& state_to_upd, std::optional<PlayerId>& player_id,
               std::string address) override {
    state_to_upd.tryToJoin(player_id, {name, address});
    if (player_id) {
      return true;
    }
    return false;
  }

 bool amIJoin() override {
    return true;
  }
};

class ClientPlaceBomb : public ClientMessage {
 public:
  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return std::make_shared<ClientPlaceBomb>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit ClientPlaceBomb([[maybe_unused]] ByteStream& stream){};

  void updateServerState(ServerState& state_to_upd, PlayerId id,
                         std::shared_ptr<Turn> cur_turn) override {
    uint32_t bomb_id = state_to_upd.placeBomb(state_to_upd.getPlayerPos(id));
    cur_turn->addEvent(
        std::make_shared<BombPlaced>(bomb_id, state_to_upd.getPlayerPos(id)));
  }
};

class ClientPlaceBlock : public ClientMessage {
 public:
  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return std::make_shared<ClientPlaceBlock>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit ClientPlaceBlock([[maybe_unused]] ByteStream& stream){};

  void updateServerState(ServerState& state_to_upd, PlayerId id,
                         std::shared_ptr<Turn> cur_turn) override {
    if (state_to_upd.placeBlock(state_to_upd.getPlayerPos(id))) {
      cur_turn->addEvent(
          std::make_shared<BlockPlaced>(state_to_upd.getPlayerPos(id)));
    }
  }
};

class ClientMove : public ClientMessage {
 private:
  uint8_t direction{};

 public:
  static std::shared_ptr<ClientMessage> create(ByteStream& rest) {
    return std::make_shared<ClientMove>(rest);
  }

  /**
   * Constructor that creates the object from a specified bytesream
   * (deserializes the message on the go)
   */
  explicit ClientMove(ByteStream& stream) {
    stream >> direction;
  };

  void updateServerState(ServerState& state_to_upd, PlayerId id,
                         std::shared_ptr<Turn> cur_turn) override {
    if (state_to_upd.movePlayerInDirection(id, direction)) {
      cur_turn->addEvent(
          std::make_shared<PlayerMoved>(id, state_to_upd.getPlayerPos(id)));
    }
  }
};

/**
 * Needs to be called before serialization - it is needed to populate
 * factories' function pointer map.
 */
void registerAllClient() {
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
  ServerMessage::register_to_map(4, GameEnded::create);
}

void registerAllServer() {
  ClientMessage::register_to_map(0, ClientJoin::create);
  ClientMessage::register_to_map(1, ClientPlaceBomb::create);
  ClientMessage::register_to_map(2, ClientPlaceBlock::create);
  ClientMessage::register_to_map(3, ClientMove::create);
}

#endif  // SIK_ZAD3_CLIENTSERIALIZATION_H
