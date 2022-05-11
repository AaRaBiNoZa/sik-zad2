//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_SERVERSTATE_H
#define SIK_ZAD3_SERVERSTATE_H

#include <cstdint>
#include "Messages.h"
#include "Randomizer.h"



class ServerGameState {
 private:
  uint16_t turn;
  std::vector<std::shared_ptr<Event>> events;
  std::map<Message::PlayerId, std::shared_ptr<Position>> player_positions;
  std::map<Message::PlayerId, Message::Score> scores;
  std::vector<std::shared_ptr<Bomb>> bombs;
  std::vector<std::shared_ptr<Position>> blocks;
 public:
  ServerGameState() : turn(0) {};
};

class ServerState {
 private:
  std::vector<std::shared_ptr<Player>> players;
  Randomizer rand;
  ServerGameState server_game_state;
  bool game_on;
 public:
  explicit ServerState(uint32_t seed) : rand(seed), game_on(false) {};
};

#endif  // SIK_ZAD3_SERVERSTATE_H
