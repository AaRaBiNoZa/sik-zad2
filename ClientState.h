//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_CLIENTSTATE_H
#define SIK_ZAD3_CLIENTSTATE_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include "utils.h"

struct ClientCommandLineOpts {
  std::string display_address;
  std::string player_name;
  uint16_t port;
  std::string server_address;
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
  bool am_i_playing = false;

  void addBomb(BombId id, Position pos) {
    bombs[id] = {std::move(pos), bomb_timer};
  }
};


#endif  // SIK_ZAD3_CLIENTSTATE_H
