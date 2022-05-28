#ifndef SIK_ZAD2_SERVERSTATE_H
#define SIK_ZAD2_SERVERSTATE_H

#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <optional>

#include "utils.h"
#include "Randomizer.h"

class Event;
class ClientMessage;

namespace po = boost::program_options;

class GameState {
 public:
  std::atomic_uint16_t  turn;
  uint32_t next_bomb_id{};
  std::vector<std::vector<std::shared_ptr<Event>>> all_turns;
  std::map<PlayerId, Position> positions;
  std::map<PlayerId, Score> scores;
  std::map<BombId, Bomb> bombs;
  std::set<Position> blocks;
  std::vector<std::optional<std::shared_ptr<ClientMessage>>> messages_from_this_turn;

  explicit GameState(uint8_t players_count) : messages_from_this_turn(players_count){

  }

};

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
          "turn-duration,d",
          po::value<uint64_t>(&turn_duration)->required())(
          "explosion-radius,e",
          po::value<uint16_t>(&explosion_radius)->required())(
          "initial-blocks,k",
          po::value<uint16_t>(&initial_blocks)->required())(
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
        seed = (uint32_t) std::chrono::system_clock::now().time_since_epoch().count();
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
  std::vector<PlayerInfo> players;
  Randomizer rand;
  uint16_t bomb_timer;
  //  uint8_t players_count;
  std::atomic_uint8_t next_player_id;
  uint64_t turn_duration;
  uint16_t explosion_radius;
  uint16_t initial_blocks;
  uint16_t size_x;
  uint16_t game_length;
  std::optional<GameState> game_state;

  uint16_t size_y;
 public:
  void try_to_join(std::optional<PlayerId>& id) {
    PlayerId possible_id = next_player_id++;
    if (possible_id < players.size()) {
      id = possible_id;
    } else {
      next_player_id--;
    }
  }
  const std::string &get_server_name() const {
    return server_name;
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

  uint8_t get_players_count() const {
      return players.size();
  };
  explicit ServerState(ServerCommandLineOpts opts)
      : server_name(opts.server_name),
        players(opts.players_count),
        rand(opts.seed),
        bomb_timer(opts.bomb_timer),
        turn_duration(opts.turn_duration),
        explosion_radius(opts.explosion_radius),
        initial_blocks(opts.initial_blocks),
        size_x(opts.size_x),
        game_length(opts.game_length), size_y(opts.size_y){};
};


#endif  // SIK_ZAD2_SERVERSTATE_H
