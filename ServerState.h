#ifndef SIK_ZAD2_SERVERSTATE_H
#define SIK_ZAD2_SERVERSTATE_H

#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "Randomizer.h"
#include "utils.h"

class Turn;
class ClientMessage;
class Event;

namespace po = boost::program_options;


struct ServerCommandLineOpts {
  uint16_t bomb_timer{};
  uint8_t players_count{};
  uint64_t turn_duration{};
  uint16_t explosion_radius{};
  uint16_t initial_blocks{};
  uint16_t game_length{};
  std::string server_name;
  uint16_t port{};
  uint32_t seed{};
  uint16_t size_x{};
  uint16_t size_y{};

  bool parse_command_line(int argc, char *argv[]) {
    try {
      po::options_description desc("Allowed options");
      desc.add_options()("bomb-timer,b",
                         po::value<uint16_t>(&bomb_timer)->required())(
          "help,h", "produce help message")(
          "players-count,c", po::value<uint8_t>(&players_count)->required())(
          "port,p", po::value<uint16_t>(&port)->required())(
          "turn-duration,d", po::value<uint64_t>(&turn_duration)->required())(
          "explosion-radius,e",
          po::value<uint16_t>(&explosion_radius)->required())(
          "initial-blocks,k", po::value<uint16_t>(&initial_blocks)->required())(
          "game-length,l", po::value<uint16_t>(&game_length)->required())(
          "server-name,n", po::value<std::string>(&server_name)->required())(
          "seed,s", po::value<uint32_t>(&seed))(
          "size-x,x", po::value<uint16_t>(&size_x)->required())(
          "size-y,y", po::value<uint16_t>(&size_y)->required());

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help")) {
        std::cout << desc << "\n";
        return false;
      }
      if (!vm.count("seed")) {
        seed = (uint32_t)std::chrono::system_clock::now()
                   .time_since_epoch()
                   .count();
      }
      po::notify(vm);

    } catch (std::exception &e) {
      std::cerr << "Error: " << e.what() << "\n";
      return false;
    } catch (...) {
      std::cerr << "Unknown error!"
                << "\n";
      return false;
    }
    return true;
  }
};

class ServerState {
 private:
  std::string server_name;
  std::map<PlayerId, Player> players;
  Randomizer rand;
  uint16_t bomb_timer;
  uint8_t players_count;
  std::atomic_uint8_t next_player_id;
  uint64_t turn_duration;
  uint16_t explosion_radius;
  uint16_t initial_blocks;
  uint16_t size_x;
  uint16_t game_length;
  uint16_t size_y;
  std::shared_mutex players_mut;

  /* ------------------------------------- */

  std::atomic<bool> game_started;

  std::atomic_uint16_t turn;
  uint32_t next_bomb_id{};
  std::vector<std::shared_ptr<Turn>> all_turns;
  std::map<PlayerId, Position> positions;
  std::map<PlayerId, Score> scores;
  std::map<BombId, Bomb> bombs;
  std::set<Position> blocks;
  std::map<PlayerId, std::shared_ptr<ClientMessage>>
      messages_from_this_turn;
  std::shared_mutex turn_vec_mut;
  std::atomic<uint8_t> want_to_write_to_rcv_events;
  std::atomic<uint8_t> want_to_write_to_turns;
  std::shared_mutex client_messages_rw;
  
  std::set<PlayerId> would_die;
  std::set<Position> blocks_destroyed;


 public:
  void try_to_join(std::optional<PlayerId> &id, Player p) {
    PlayerId possible_id = next_player_id++;
    if (possible_id < players_count) {
      id = possible_id;
      std::lock_guard lk(players_mut);
      players[possible_id] = std::move(p);
    } else {
      next_player_id--;
    }
  }
  std::atomic<uint8_t> &get_want_to_write_to_turns() {
    return want_to_write_to_turns;
  }
  std::shared_mutex &get_client_messages_rw() {
    return client_messages_rw;
  }
  std::map<PlayerId, std::shared_ptr<ClientMessage>>& getMessagesFromTurn() {
    return messages_from_this_turn;
  }
  Player getPlayer(PlayerId id) {
    std::lock_guard lk(players_mut);
    return players[id];
  }
  uint16_t get_initial_blocks() const {
    return initial_blocks;
  }
  uint64_t get_turn_duration() const {
    return turn_duration;
  }
  void addBlock(Position pos) {
    blocks.insert(pos);
  }

  void movePlayer(PlayerId id, Position pos) {
    positions[id] = pos;
  }

  bool movePlayerTo(PlayerId id, uint8_t dir) {
    Position pos = positions[id];
    switch (dir) {
      case 0:
        pos.y++;
        break;
      case 1:
        pos.x++;
        break;
      case 2:
        pos.y--;
        break;
      case 3:
        pos.x--;
        break;
      default:
        return false;
    }
    std::cerr << "YOU MOTHERFUKER : " <<  blocks.contains(pos);
    std::cerr << '\n' << pos.x << ":" << pos.y << '\n';
    for (auto k: blocks) {
      std::cerr << k.x << " : " << k.y << '\n';
    }
    if (blocks.contains(pos)) {
      return false;
    } else if (pos.x >= size_x || pos.y >= size_y || pos.x < 0 || pos.y < 0 ) {
      return false;
    }
    positions[id] = pos;
    return true;
  }
  std::map<BombId, Bomb> get_bombs() {
    return bombs;
  }

  std::set<PlayerId> cleanUpBombs() {
    for (auto k: would_die) {
      scores[k]++;
      messages_from_this_turn[k].reset();
    }
    for (auto k: blocks_destroyed) {
      blocks.erase(k);
    }
    auto res = would_die;
    would_die.clear();
    blocks_destroyed.clear();

    return res;
  }
void resetmessagesfromturn() {
  messages_from_this_turn.clear();
}
std::optional<std::pair<std::set<Position>, std::set<PlayerId>>> checkBomb(BombId id) {
    bombs[id].timer--;
    Position pos = bombs[id].position;
    std::set<Position> blocks_destroyed_local;
    std::set<PlayerId> would_die_local;
    if (bombs[id].timer == 0) {
      bombs.erase(id);
      
      for (auto [id, player_pos]: positions) {
        if (player_pos == pos) {
          would_die_local.insert(id);
        }
      }
      if (blocks.contains(pos)) {
        blocks_destroyed_local.insert(pos);
        for (auto k: would_die_local) {
          would_die.insert(k);
        }
        for (auto pos: blocks_destroyed_local) {
          blocks_destroyed.insert(pos);
        }
        return std::optional<std::pair<std::set<Position>, std::set<PlayerId>>>({blocks_destroyed_local, would_die_local});
      }

      for (uint16_t i = 1; i < explosion_radius + 1 && pos.y + i < size_y;
           ++i) {
        Position temp(pos.x, pos.y + i);
        for (auto [player_id, player_pos]: positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (uint16_t i = 1; i < explosion_radius + 1 && pos.y - i >= 0;
           ++i) {
        Position temp(pos.x, pos.y - i);
        for (auto [player_id, player_pos]: positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (uint16_t i = 1; i < explosion_radius + 1 && pos.x + i < size_x;
           ++i) {
        Position temp(pos.x + i, pos.y);
        for (auto [player_id, player_pos]: positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (uint16_t i = 1; i < explosion_radius + 1 && pos.x - i >= 0;
           ++i) {
        Position temp(pos.x - i, pos.y);
        for (auto [player_id, player_pos]: positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (auto k: would_die_local) {
        would_die.insert(k);
      }
      for (auto pos: blocks_destroyed_local) {
        blocks_destroyed.insert(pos);
      }
      return std::optional<std::pair<std::set<Position>, std::set<PlayerId>>>({blocks_destroyed_local, would_die_local});

    }
    return std::optional<std::pair<std::set<Position>, std::set<PlayerId>>>();
  }

  bool placeBlock(Position pos) {
    if (blocks.contains(pos)) {
      return false;
    }
    blocks.insert(pos);
    return true;
  }

  uint32_t placeBomb(Position pos) {
    bombs.insert({next_bomb_id++, {pos, bomb_timer}});
    return next_bomb_id - 1;
  }


  Position getPlayerPos(PlayerId id) {
    return positions[id];
  }

//  std::shared_ptr<Turn> getCurTurn() {
//    return all_turns[turn];
//  }
//  void setTurn(uint16_t turn_num) {
//    turn = turn_num;
//  }

  void addTurn(std::shared_ptr<Turn> t) {
    want_to_write_to_turns++;
    std::unique_lock lk(turn_vec_mut);
    all_turns.push_back(t);
    want_to_write_to_turns--;
  }

  const std::vector<std::shared_ptr<Turn>> &get_all_turns() const {
    return all_turns;
  }
  std::atomic<uint8_t> &get_want_to_write_to_rcv_events() {
    return want_to_write_to_rcv_events;
  }
  const std::string &get_server_name() const {
    return server_name;
  }
  void start_game() {
    game_started = true;
  }

  uint16_t get_bomb_timer() const {
    return bomb_timer;
  }
  uint16_t get_explosion_radius() const {
    return explosion_radius;
  }
  uint16_t get_size_x() const {
    return size_x;
  }
  uint16_t get_game_length() const {
    return game_length;
  }
  uint16_t get_size_y() const {
    return size_y;
  }
  void setPlayerPos(PlayerId id, Position pos) {
    positions[id] = pos;
  }

  uint8_t get_players_count() const {
    return players_count;
  };

  std::map<PlayerId, Player> &get_players() {
    return players;
  }

  const std::atomic<bool> &get_game_started() const {
    return game_started;
  }

  [[nodiscard]] std::shared_mutex &get_players_mut() {
    return players_mut;
  }

  [[nodiscard]] std::shared_mutex &get_turn_vec_mut() {
    return turn_vec_mut;
  }
  Randomizer &get_rand() {
    return rand;
  }
  explicit ServerState(ServerCommandLineOpts opts)
      : server_name(opts.server_name),
        players(),
        rand(opts.seed),
        bomb_timer(opts.bomb_timer),
        players_count(opts.players_count),
        turn_duration(opts.turn_duration),
        explosion_radius(opts.explosion_radius),
        initial_blocks(opts.initial_blocks),
        size_x(opts.size_x),
        game_length(opts.game_length),
        size_y(opts.size_y),
        game_started(false){};
};


//class GameState {
// public:
//
//  ServerState server_state;
//
//      explicit GameState(std::map<PlayerId, Player> players, Randomizer& rand, ServerState s_state) : players(players), rand(rand), server_state(s_state) {
//    all_turns.emplace_back();
//  }
//  const std::map<PlayerId, Player> &get_players() const {
//    return players;
//  }
//};

#endif  // SIK_ZAD2_SERVERSTATE_H
