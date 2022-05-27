#include <iostream>
#include <string>

#include "ByteStream.h"
#include "Server.h"
#include "ServerState.h"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  ServerCommandLineOpts opts;
  bool parse_results = opts.parse_command_line(argc, argv);
  opts.players_count -= '0';
  if (!parse_results) {
    return 1;
  }
  try {
    boost::asio::io_context io_context;
    Server serv(io_context, opts);
    //    io_context.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}