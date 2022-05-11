//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_CLIENT_H
#define SIK_ZAD3_CLIENT_H

#include "ClientState.h"
#include "TcpClientConnection.h"
#include "UdpServer.h"

struct ClientCommandLineOpts {
  std::string display_address;
  std::string player_name;
  uint16_t port;
  std::string server_address;
};

class Client {
  UdpServer udp_display;
  TcpClientConnection tcp_server;
  ClientState aggregated_state;
 public:
  Client(boost::asio::io_context& io_context, ClientCommandLineOpts opts) : udp_display(io_context, opts.port, opts.display_address),
        tcp_server(io_context, opts.port, opts.server_address) {};
};

#endif  // SIK_ZAD3_CLIENT_H
