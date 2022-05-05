#include <iostream>
#include <string>

#include "ClientConfig.h"
#include "UdpServer.h"
#include "boost/program_options.hpp"
#include "TcpClientConnection.h"
#include "Messages.h"

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

  Message::message_map().insert({'0', InputMessage::create});
  InputMessage::input_message_map().insert({'0', PlaceBomb::create});
  std::stringstream x;
  x << "0000";
  Message pbomb = *Message::unserialize(x);
  pbomb.say_hello();

  return 0;
}
