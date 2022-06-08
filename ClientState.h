#ifndef SIK_ZAD3_CLIENTSTATE_H
#define SIK_ZAD3_CLIENTSTATE_H

#include <boost/program_options.hpp>
#include <iostream>
#include <map>
#include <set>
#include <string>

#include "utils.h"

namespace po = boost::program_options;

/**
 * Struct created for the sole purpose of parsing and saving
 * command line options.
 * If help is used, all other options are optional (we won't run).
 * If no help is detected, all of the other options are required.
 */
struct ClientCommandLineOpts {
  std::string display_address;
  std::string player_name;
  uint16_t port;
  std::string server_address;

  bool parse_command_line(int argc, char *argv[]) {
    try {
      po::options_description desc("Opcje programu:");
      desc.add_options()
          ("gui-address,d", po::value<std::string>(&display_address)->required(),
          "<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>")
          ("help,h", "Wypisuje jak używać programu")
          ("player-name,n", po::value<std::string>(&player_name)->required(),
          "<String>")
          ("port,p", po::value<uint16_t>(&port)->required(),"<u16>")
          ("server-address,s",po::value<std::string>(&server_address)->required(),
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

/**
 * Struct for storing ClientState.
 * At all times there will only be one ClientState (for one Client).
 * It will never be used polymorphically nor will it have subclasses.
 * It's main function is to just store data, that's why I chose struct over
 * class.
 */
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
  std::set<Position> explosions;
  std::map<PlayerId, Score> scores;
  std::set<PlayerId> would_die;
  std::set<Position> blocks_to_destroy;

  bool game_on = false;

  /**
   * Ease function to add new bomb (we have the bomb timer ready)
   */
  void add_bomb(BombId id, Position pos) {
    bombs[id] = {pos, bomb_timer};
  }

  /**
   * Function that calculates current round's explosions.
   * It goes explosion_radius - 1 times to all 4 sides, unless it
   * finds a block, then it will be the last explosion position in that
   * direction.
   * If a bomb exploded on a block, we only insert one explosion - on that
   * blocks position.
   */
  void calculate_explosions(Position explosion) {
    explosions.insert(explosion);
    if (blocks.contains(explosion)) {
      return;
    }

    for (uint16_t i = 1; i < explosion_radius + 1 && explosion.y + i < size_y;
         ++i) {
      Position temp(explosion.x, explosion.y + i);
      explosions.insert(temp);
      if (blocks.contains(temp)) {
        break;
      }
    }

    for (uint16_t i = 1; i < explosion_radius + 1 && explosion.y - i >= 0;
         ++i) {
      Position temp(explosion.x, explosion.y - i);
      explosions.insert(temp);
      if (blocks.contains(temp)) {
        break;
      }
    }

    for (uint16_t i = 1; i < explosion_radius + 1 && explosion.x + i < size_x;
         ++i) {
      Position temp(explosion.x + i, explosion.y);
      explosions.insert(temp);
      if (blocks.contains(temp)) {
        break;
      }
    }

    for (uint16_t i = 1; i < explosion_radius + 1 && explosion.x - i >= 0;
         ++i) {
      Position temp(explosion.x - i, explosion.y);
      explosions.insert(temp);
      if (blocks.contains(temp)) {
        break;
      }
    }
  }

  /**
   * Prepares the clientstate for a new game (on the same server).
   */
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
