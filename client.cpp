#include "Client.h"

#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include "ClientState.h"
#include "Message.h"

int main(int argc, char *argv[]) {
  ClientCommandLineOpts opts;
  if (!opts.parse_command_line(argc, argv)) {
    return 1;
  }
  register_all_client();

  try {
    boost::asio::io_context io_context;
    Client client(io_context, opts);
    io_context.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
