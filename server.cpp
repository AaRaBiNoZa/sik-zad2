#include "Server.h"

#include <iostream>
#include <string>

#include "ByteStream.h"
#include "ServerState.h"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  ServerCommandLineOpts opts;

  if (!opts.parse_command_line(argc, argv) || !opts.validate()) {
    return 1;
  }
  register_all_server();
  try {
    boost::asio::io_context io_context;
    Server serv(io_context, opts);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}