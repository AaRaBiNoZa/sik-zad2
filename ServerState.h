//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_SERVERSTATE_H
#define SIK_ZAD3_SERVERSTATE_H

#include <cstdint>
#include <shared_mutex>

#include "Randomizer.h"

class Event;
class ClientMessage;

struct PlayerInfo {
  std::string name;
  tcp::endpoint endpoint;
};

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

  };
  uint32_t last_bomb_id() {
    return next_bomb_id - 1;
  }

  void addMessage(PlayerId id, std::shared_ptr<ClientMessage> mess) {
    messages_from_this_turn[id] = mess;
  }

  uint32_t addBomb(PlayerId id, uint16_t timer) {
    bombs.insert({next_bomb_id, {positions[id], timer}});
    return next_bomb_id++;
  }

  //  void addBlock(PlayerId id) {
  //    blocks.insert(positions[id]);
  //  }

  void moveUp(PlayerId id, uint16_t size_y) {
    if (positions[id].y < size_y - 1) {
      positions[id].y++;
    }
  }

  void moveRight(PlayerId id, uint16_t size_x) {
    if (positions[id].x < size_x - 1) {
      positions[id].x++;
    }
  }

  void moveDown(PlayerId id, uint16_t size_y) {
    if (positions[id].y > size_y + 1) {
      positions[id].y--;
    }
  }
  void moveLeft(PlayerId id, uint16_t size_x) {
    if (positions[id].y < size_x + 1) {
      positions[id].x--;
    }
  }

  void addEvent(std::shared_ptr<Event> event) {
    all_turns[turn].push_back(event);
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
};

class ServerState {
 public:
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
  std::shared_mutex players_mutex;
  std::optional<GameState> game_state;

  uint16_t size_y;
 public:
  uint32_t addBomb(PlayerId id) {
    return game_state->addBomb(id, bomb_timer);
  }

  uint8_t getMaxCount() {
    return players.size();
  }

  void addBlock(PlayerId id) {
    game_state->blocks.insert(getPlayerPosition(id));
  }

  void addEvent(std::shared_ptr<Event> event) {
    game_state->addEvent(event);
  }

  Position getPlayerPosition(PlayerId id) {
    return game_state->positions[id];
  }

  Position move(uint8_t direction, PlayerId id) {
    switch (direction) {
      case 0:
        game_state->moveUp(id, size_y);
        break;
      case 1:
        game_state->moveRight(id, size_x);
        break;
      case 2:
        game_state->moveDown(id, size_y);
        break;
      case 3:
        game_state->moveLeft(id, size_x);
        break;
      default:
        std::cerr << "WRONG DIR";
        exit(1);
    }

    return getPlayerPosition(id);
  }

  void startGame() {
    game_state.emplace(players.size());
  }

  explicit ServerState(ServerCommandLineOpts opts)
      : players(opts.players_count),
        rand(opts.seed),
        bomb_timer(opts.bomb_timer),
        turn_duration(opts.turn_duration),
        explosion_radius(opts.explosion_radius),
        initial_blocks(opts.initial_blocks),
        size_x(opts.size_x),
        size_y(opts.size_y), game_length(opts.game_length), players_mutex(){};
};


#endif  // SIK_ZAD3_SERVERSTATE_H
