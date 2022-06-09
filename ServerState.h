#ifndef SIK_ZAD2_SERVERSTATE_H
#define SIK_ZAD2_SERVERSTATE_H

#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <string>
#include <utility>

#include "MessageUtils.h"
#include "Randomizer.h"

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
      po::options_description desc("Opcje programu");
      desc.add_options()
          ("bomb-timer,b",po::value<uint16_t>(&bomb_timer)->required(), "<u16>")
          ("help,h", "produce help message")
          ("players-count,c", po::value<uint8_t>(&players_count)->required(),
                   "<u8>")
          ("port,p", po::value<uint16_t>(&port)->required(), "<u16>")
          ("turn-duration,d", po::value<uint64_t>(&turn_duration)->required(),
                           "<u64, milisekundy>")
          ("explosion-radius,e",po::value<uint16_t>(&explosion_radius)->required(),
                               "<u16>")(
          "initial-blocks,k", po::value<uint16_t>(&initial_blocks)->required(),
                                  "<u16>")
          ("game-length,l", po::value<uint16_t>(&game_length)->required(),
                                   "<u16>")
          ("server-name,n", po::value<std::string>(&server_name)->required(),
                                       "<String>")
          ("seed,s", po::value<uint32_t>(&seed), "<u32, parametr opcjonalny>")
          ("size-x,x", po::value<uint16_t>(&size_x)->required(), "<u16>")
          ("size-y,y", po::value<uint16_t>(&size_y)->required(), "<u16>");

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

struct ServerConfiguration {
  const std::string server_name;
  const uint16_t bomb_timer;
  const uint8_t players_count;
  const uint64_t turn_duration;
  const uint16_t explosion_radius;
  const uint16_t initial_blocks;
  const uint16_t game_length;
  const uint16_t port;
  const uint32_t seed;
  const uint16_t size_x;
  const uint16_t size_y;

  explicit ServerConfiguration(ServerCommandLineOpts &opts)
      : server_name(std::move(opts.server_name)),
        bomb_timer(opts.bomb_timer),
        players_count(opts.players_count),
        turn_duration(opts.turn_duration),
        explosion_radius(opts.explosion_radius),
        initial_blocks(opts.initial_blocks),
        game_length(opts.game_length),
        port(opts.port),
        seed(opts.seed),
        size_x(opts.size_x),
        size_y(opts.size_y) {
  }
};

// Reader writer lock, but there is only one "writer", so I just
// mark if he wants to write with atomic variable
// also - when it comes to client messages, clients(writers) can actually
// do it simultaneously, since every player has his or her own
// index.
// When the server tries to collect the data, then we got a problem and
// have to get exclusive access (cause server wants to read the whole thing
// and no one can be able to do any changes)
struct Synchronizer {
  using rw_mutex = std::shared_mutex;

  rw_mutex client_messages_rw;
  rw_mutex players_rw;
  rw_mutex turns_rw;

  std::condition_variable_any wait_for_shared_turns;
  std::condition_variable_any wait_for_shared_client_messages;
  std::condition_variable_any wait_for_shared_players;

  std::atomic<uint8_t> want_to_write_to_players;
  std::atomic<uint8_t> want_to_write_to_client_messages;
  std::atomic<uint8_t> want_to_write_to_turns;

  Synchronizer()
      : client_messages_rw(),
        players_rw(),
        turns_rw(),
        wait_for_shared_turns(),
        wait_for_shared_client_messages(),
        wait_for_shared_players(),
        want_to_write_to_players(),
        want_to_write_to_client_messages(),
        want_to_write_to_turns() {
  }
};

class ServerState {
 private:
  const ServerConfiguration server_config;
  std::map<PlayerId, Player> players;
  Randomizer rand;
  uint32_t next_bomb_id{};
  std::vector<std::shared_ptr<Turn>> all_turns;
  std::map<PlayerId, Position> positions;
  std::map<PlayerId, Score> scores;
  std::map<BombId, Bomb> bombs;
  std::set<Position> blocks;
  std::map<PlayerId, std::shared_ptr<ClientMessage>> messages_from_this_turn;
  std::set<PlayerId> would_die;
  std::set<Position> blocks_destroyed;
  std::atomic_uint8_t next_player_id;
  std::atomic<bool> game_started;

  Synchronizer synchro;

 public:
  void reset() {
    synchro.want_to_write_to_turns++;
    synchro.want_to_write_to_players++;
    std::unique_lock turns_lock(synchro.turns_rw);
    std::unique_lock players_lock(synchro.players_rw);

    players.clear();
    next_player_id = 0;
    game_started = false;
    next_bomb_id = 0;
    all_turns.clear();
    positions.clear();
    scores.clear();
    bombs.clear();
    blocks.clear();
    messages_from_this_turn.clear();
    would_die.clear();
    blocks_destroyed.clear();

    synchro.want_to_write_to_turns--;
    synchro.want_to_write_to_players--;
    wake_waiting_for_shared_turn();
    wake_waiting_for_shared_players();
  }

  void try_to_join(std::optional<PlayerId> &id, Player p) {
    PlayerId possible_id = next_player_id++;
    if (possible_id < server_config.players_count) {
      id = possible_id;
      synchro.want_to_write_to_players++;
      std::unique_lock lk(synchro.players_rw);
      players[possible_id] = std::move(p);
      synchro.want_to_write_to_players--;
      lk.unlock();
      wake_waiting_for_shared_players();

    } else {
      next_player_id--;
    }
  }

  // functions that access resources that are all accessed only by one thread
  // Server thread. They need no sync cause are done synchronously by nature
  [[nodiscard]] uint16_t get_initial_blocks() const {
    return server_config.initial_blocks;
  }
  [[nodiscard]] uint64_t get_turn_duration() const {
    return server_config.turn_duration;
  }

  void move_player(PlayerId id, Position pos) {
    positions[id] = pos;
  }

  bool move_player_in_direction(PlayerId id, uint8_t dir) {
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

    if (blocks.contains(pos)) {
      return false;
    } else if (pos.x >= server_config.size_x || pos.y >= server_config.size_y ||
               pos.x == UINT16_MAX ||
               pos.y == UINT16_MAX) {  // these can't be negative
      return false;
    }
    positions[id] = pos;
    return true;
  }

  std::map<BombId, Bomb> get_bombs() {
    return bombs;
  }

  std::set<PlayerId> clean_up_bombs() {
    for (auto k : would_die) {
      scores[k]++;
      messages_from_this_turn[k].reset();
    }
    for (auto k : blocks_destroyed) {
      blocks.erase(k);
    }
    auto res = would_die;
    would_die.clear();
    blocks_destroyed.clear();

    return res;
  }

  std::map<PlayerId, Score> get_scores() {
    return scores;
  }

  std::optional<std::pair<std::set<Position>, std::set<PlayerId>>> check_bomb(
      BombId id) {
    bombs[id].timer--;
    Position pos = bombs[id].position;
    std::set<Position> blocks_destroyed_local;
    std::set<PlayerId> would_die_local;
    if (bombs[id].timer == 0) {
      bombs.erase(id);

      for (auto [player_id, player_pos] : positions) {
        if (player_pos == pos) {
          would_die_local.insert(player_id);
        }
      }
      if (blocks.contains(pos)) {
        blocks_destroyed_local.insert(pos);
        for (auto k : would_die_local) {
          would_die.insert(k);
        }
        for (auto block_pos : blocks_destroyed_local) {
          blocks_destroyed.insert(block_pos);
        }
        return std::optional<std::pair<std::set<Position>, std::set<PlayerId>>>(
            {blocks_destroyed_local, would_die_local});
      }

      for (uint16_t i = 1; i < server_config.explosion_radius + 1 &&
                           pos.y + i < server_config.size_y;
           ++i) {
        Position temp(pos.x, pos.y + i);
        for (auto [player_id, player_pos] : positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (uint16_t i = 1;
           i < server_config.explosion_radius + 1 && pos.y - i >= 0; ++i) {
        Position temp(pos.x, pos.y - i);
        for (auto [player_id, player_pos] : positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (uint16_t i = 1; i < server_config.explosion_radius + 1 &&
                           pos.x + i < server_config.size_x;
           ++i) {
        Position temp(pos.x + i, pos.y);
        for (auto [player_id, player_pos] : positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (uint16_t i = 1;
           i < server_config.explosion_radius + 1 && pos.x - i >= 0; ++i) {
        Position temp(pos.x - i, pos.y);
        for (auto [player_id, player_pos] : positions) {
          if (player_pos == temp) {
            would_die_local.insert(player_id);
          }
        }
        if (blocks.contains(temp)) {
          blocks_destroyed_local.insert(temp);
          break;
        }
      }

      for (auto k : would_die_local) {
        would_die.insert(k);
      }
      for (auto block_pos : blocks_destroyed_local) {
        blocks_destroyed.insert(block_pos);
      }
      return std::optional<std::pair<std::set<Position>, std::set<PlayerId>>>(
          {blocks_destroyed_local, would_die_local});
    }
    return std::optional<std::pair<std::set<Position>, std::set<PlayerId>>>();
  }

  bool place_block(Position pos) {
    if (blocks.contains(pos)) {
      return false;
    }
    blocks.insert(pos);
    return true;
  }

  uint32_t place_bomb(Position pos) {
    bombs.insert({next_bomb_id++, {pos, server_config.bomb_timer}});
    return next_bomb_id - 1;
  }

  Position get_player_pos(PlayerId id) {
    return positions[id];
  }

  const std::string &get_server_name() const {
    return server_config.server_name;
  }

  void start_game() {
    game_started = true;
  }

  [[nodiscard]] uint16_t get_bomb_timer() const {
    return server_config.bomb_timer;
  }
  [[nodiscard]] uint16_t get_explosion_radius() const {
    return server_config.explosion_radius;
  }
  [[nodiscard]] uint16_t get_size_x() const {
    return server_config.size_x;
  }
  [[nodiscard]] uint16_t get_game_length() const {
    return server_config.game_length;
  }
  [[nodiscard]] uint16_t get_size_y() const {
    return server_config.size_y;
  }

  [[nodiscard]] uint8_t get_players_count() const {
    return server_config.players_count;
  };

  Randomizer &get_rand() {
    return rand;
  }

  // Functions that have to do something with sync

  std::shared_mutex &get_client_messages_rw() {
    return synchro.client_messages_rw;
  }

  std::condition_variable_any &get_wait_for_players_messages() {
    return synchro.wait_for_shared_client_messages;
  }

  std::map<PlayerId, std::shared_ptr<ClientMessage>>
      &get_messages_from_turn_no_sync() {
    return messages_from_this_turn;
  }
  Player get_player_sync(PlayerId id) {
    std::lock_guard lk(synchro.players_rw);
    return players[id];
  }

  void reset_messages_from_players_no_sync() {
    messages_from_this_turn.clear();
  }

  void add_turn_sync(std::shared_ptr<Turn> t) {
    synchro.want_to_write_to_turns++;
    std::unique_lock lk(synchro.turns_rw);
    all_turns.push_back(t);
    synchro.want_to_write_to_turns--;
    lk.unlock();
    wake_waiting_for_shared_turn();
  }

  const std::vector<std::shared_ptr<Turn>> &get_all_turns_no_sync() const {
    return all_turns;
  }

  std::map<PlayerId, Player> &get_players() {
    return players;
  }

  const std::atomic<bool> &get_game_started() const {
    return game_started;
  }

  [[nodiscard]] std::shared_mutex &get_players_mutex() {
    return synchro.players_rw;
  }

  [[nodiscard]] std::shared_mutex &get_all_turns_mutex() {
    return synchro.turns_rw;
  }

  [[nodiscard]] std::shared_mutex &get_client_messages_mutex() {
    return synchro.client_messages_rw;
  }

  std::condition_variable_any &get_wait_for_turns() {
    return synchro.wait_for_shared_turns;
  }

  std::atomic<uint8_t> &get_want_to_write_to_turns() {
    return synchro.want_to_write_to_turns;
  }

  std::condition_variable_any &get_wait_for_players() {
    return synchro.wait_for_shared_players;
  }

  std::atomic<uint8_t> &get_want_to_write_to_players() {
    return synchro.want_to_write_to_players;
  }

  std::atomic<uint8_t> &get_want_to_write_to_client_messages() {
    return synchro.want_to_write_to_client_messages;
  }

  void wake_waiting_for_shared_client_messages() {
    synchro.wait_for_shared_client_messages.notify_all();
  }

  void wake_waiting_for_shared_turn() {
    synchro.wait_for_shared_turns.notify_all();
  }

  void wake_waiting_for_shared_players() {
    synchro.wait_for_shared_players.notify_all();
  }

  explicit ServerState(ServerCommandLineOpts opts)
      : server_config(opts), rand(server_config.seed), game_started(false) {
  }
};

#endif  // SIK_ZAD2_SERVERSTATE_H
