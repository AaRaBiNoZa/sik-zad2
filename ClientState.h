//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_CLIENTSTATE_H
#define SIK_ZAD3_CLIENTSTATE_H

#include "Messages.h"
class ClientState {
 private:
  DrawMessage Lobby;
  DrawMessage Game;
  bool game_on;
 public:
  ClientState() : game_on(false) {};
};

#endif  // SIK_ZAD3_CLIENTSTATE_H
