//
// Created by ciriubuntu on 11.05.22.
//

#include <iostream>
#include <string>

#include "ByteStream.h"
#include "Messages.h"
#include "boost/program_options.hpp"
#include "Server.h"

namespace po = boost::program_options;

bool parse_command_line(int argc, char *argv[], ServerCommandLineOpts *out) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("bomb-timer,b",
                       po::value<uint16_t>(&out->bomb_timer)->required())(
        "help,h", "produce help message")(
        "players-count,c", po::value<uint8_t>(&out->players_count)->required())(
        "port,p", po::value<uint16_t>(&out->port)->required())(
        "turn-duration,d",
        po::value<uint64_t>(&out->turn_duration)->required())(
        "explosion-radius,e",
        po::value<uint16_t>(&out->explosion_radius)->required())(
        "initial-blocks,k",
        po::value<uint16_t>(&out->initial_blocks)->required())(
        "game-length,l", po::value<uint16_t>(&out->game_length)->required())(
        "server-name,n", po::value<std::string>(&out->server_name)->required())(
        "seed,s", po::value<uint32_t>(&out->seed))(
        "size-x,x", po::value<uint16_t>(&out->size_x)->required())(
        "size-y,y", po::value<uint16_t>(&out->size_y)->required());

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return false;
    }
    if (!vm.count("seed")) {
      out->seed = std::chrono::system_clock::now().time_since_epoch().count();
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

int main(int argc, char *argv[]) {
  ServerCommandLineOpts opts;
  bool parse_results = parse_command_line(argc, argv, &opts);
  if (!parse_results) {
    return 1;
  }
  DrawMessage::register_to_map(0, Lobby::create);
  DrawMessage::register_to_map(1, Game::create);
  InputMessage::register_to_map(0, PlaceBomb::create);
  InputMessage::register_to_map(1, PlaceBlock::create);
  InputMessage::register_to_map(2, Move::create);
  ClientMessage::register_to_map(0, Join::create);

  ServerMessage::register_to_map(0, Hello::create);
  ServerMessage::register_to_map(1, AcceptedPlayer::create);
  ServerMessage::register_to_map(2, GameStarted::create);
  ServerMessage::register_to_map(3, Turn::create);
  ServerMessage::register_to_map(4, GameEnded::create);
  Event::register_to_map(0, BombPlaced::create);
  Event::register_to_map(1, BombExploded::create);
  Event::register_to_map(2, PlayerMoved::create);
  Event::register_to_map(3, BlockPlaced::create);

  try {
    boost::asio::io_context io_context;
    Server serv(io_context, opts);
    io_context.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  //  ByteStream k;
  //  k << 1;
  //  std::string sname("SERW");
  //  uint8_t players_count{1};
  //  uint16_t size_x{2};
  //  uint16_t size_y{3};
  //  uint16_t game_length{4};
  //  uint16_t explosion_radius{5};
  //  uint16_t bomb_timer{6};
  //  std::pair<uint8_t, Player> p1{8,{"adam", "al"}};
  //  std::pair<uint8_t, Player> p2{9,{"adam", "lala"}};
  //  std::map<uint8_t, Player> players;
  //  players.insert(p1);
  //  players.insert(p2);
  // -----------------------
  //  k<< sname << players_count << size_x << size_y << game_length <<
  //  explosion_radius << bomb_timer << players; k.resetPtr();
  //  std::shared_ptr<Message> l = DrawMessage::unserialize(k);
  //  l->say_hello();
  /* game */
  //  k.resetPtr();
  //  std::map<Message::PlayerId, Position> player_positions;
  //  std::pair<Message::PlayerId, Position> player_pos1 {2, {0,0}};
  //  std::pair<Message::PlayerId, Position> player_pos2 {3, {1,1}};
  //  player_positions.insert(player_pos1);
  //  player_positions.insert(player_pos2);
  //  std::vector<Position> blocks;
  //  blocks.emplace_back(1,3);
  //  blocks.emplace_back(1,2);
  //  std::vector<Bomb> bombs;
  //  bombs.emplace_back(blocks[0],16);
  //  bombs.emplace_back(blocks[1],20);
  //  std::vector<Position> explosions = blocks;
  //  std::map<Message::PlayerId, Message::Score> scores;
  //  scores.insert({1,3});
  //  scores.insert({2,5});
  //  uint16_t turn = 20;
  //  k << 1 << sname << size_x << size_y << game_length << turn << players <<
  //  player_positions << blocks << bombs << explosions << scores; k.resetPtr();
  //  std::shared_ptr<Message> l = DrawMessage::unserialize(k);
  //  l->say_hello();
  //  std::cout << game;

  return 0;
}
