//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_CLIENTSTATE_H
#define SIK_ZAD3_CLIENTSTATE_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <boost/program_options.hpp>
#include "utils.h"

namespace po = boost::program_options;


struct ClientCommandLineOpts {
  std::string display_address;
  std::string player_name;
  uint16_t port;
  std::string server_address;

  bool parse_command_line(int argc, char *argv[]) {
    try {
      po::options_description desc("Allowed options");
      desc.add_options()(
          "display-address,d", po::value<std::string>(&display_address)->required(),
          "<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>")(
          "help,h", "produce help message")("player-name,n",
                                            po::value<std::string>(&player_name)->required())(
          "port,p", po::value<uint16_t>(&port)->required())(
          "server-address,s", po::value<std::string>(&server_address)->required(),
          "<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>");

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help")) {
        std::cout << desc << "\n";
        return false;
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

struct ClientState {
  std::string server_name;
  uint8_t players_count;
  uint16_t size_x;
  uint16_t size_y;
  uint16_t game_length;
  uint16_t explosion_radius;
  uint16_t bomb_timer;
  std::map<PlayerId, Player> players;
  uint16_t turn;
  std::map<PlayerId, Position> positions;
  std::set<Position> blocks;
  std::map<BombId, Bomb> bombs;
  std::vector<Position> explosions;
  std::map<PlayerId, Score> scores;
  std::set<PlayerId> would_die;
  bool game_on = false;

  void addBomb(BombId id, Position pos) {
    bombs[id] = {std::move(pos), bomb_timer};
  }

  void reset() {
    players.clear();
    turn = 0;
    positions.clear();
    blocks.clear();
    bombs.clear();
    explosions.clear();
    scores.clear();
    would_die.clear();
    game_on = false;
  }
};


#endif  // SIK_ZAD3_CLIENTSTATE_H
