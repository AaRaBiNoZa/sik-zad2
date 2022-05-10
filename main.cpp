#include <iostream>
#include <string>

#include "ClientConfig.h"
#include "UdpServer.h"
#include "boost/program_options.hpp"
#include "TcpClientConnection.h"
#include "Messages.h"
#include "ByteStream.h"

namespace po = boost::program_options;

bool parse_command_line(int argc, char *argv[]) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()(
        "display-address,d", po::value<std::string>()->required(),
        "<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>")(
        "help,h", "produce help message")("player-name,n",
                                          po::value<std::string>()->required())(
        "port,p", po::value<uint16_t>()->required())(
        "server-address,s", po::value<std::string>()->required(),
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

int main(int argc, char *argv[]) {
  bool parse_results = parse_command_line(argc, argv);
  if (!parse_results) {
    return 1;
  }
//
//  try {
//    boost::asio::io_context io_context;
//    UdpServer serv(io_context, 2022, "127.0.0.1:2024");
//    TcpClientConnection serv2(io_context, 2022, "127.0.0.1:2023");
//    io_context.run();
//  } catch (std::exception &e) {
//    std::cerr << e.what() << std::endl;
//  }

//  Message::message_map().insert({'0', InputMessage::create});
//  InputMessage::input_message_map().insert({'0', PlaceBomb::create});
  DrawMessage::register_to_map('0', Lobby::create);
  DrawMessage::register_to_map('1', Game::create);
  ByteStream k;
  k << '0';
  std::string sname("SERW");
  uint8_t players_count{1};
  uint16_t size_x{2};
  uint16_t size_y{3};
  uint16_t game_length{4};
  uint16_t explosion_radius{5};
  uint16_t bomb_timer{6};
  std::pair<uint8_t, Player> p1{8,{"adam", "al"}};
  std::pair<uint8_t, Player> p2{9,{"adam", "lala"}};
  std::map<uint8_t, Player> players;
  players.insert(p1);
  players.insert(p2);
  k<< sname << players_count << size_x << size_y << game_length << explosion_radius << bomb_timer << players;
  k.resetPtr();
  std::shared_ptr<DrawMessage> l = DrawMessage::unserialize(k);
  l->say_hello();
//  k<< sname;
//  k >> sname;
//  std::cout << sname;
//  std::cout << l << "\n\n";
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
//  k << sname << size_x << size_y << game_length << turn << players << player_positions << blocks << bombs << explosions << scores;
//  k.resetPtr();
//  Game game(k);
//  std::cout << game;


  return 0;
}
