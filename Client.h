//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_CLIENT_H
#define SIK_ZAD3_CLIENT_H

#include "ClientState.h"
#include "TcpClientConnection.h"
#include "UdpServer.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

struct ClientCommandLineOpts {
  std::string display_address;
  std::string player_name;
  uint16_t port;
  std::string server_address;
};

class Client {
  udp::socket udp_display_sock;
  udp::endpoint display_endpoint;
  tcp::socket tcp_server_sock;
  tcp::endpoint server_endpoint;
  ClientState aggregated_state;

 public:
  Client(boost::asio::io_context& io_context, ClientCommandLineOpts opts)
      : udp_display_sock(io_context, udp::endpoint(udp::v4(), opts.port)),
        tcp_server_sock(io_context, tcp::endpoint(tcp::v4(), opts.port)) {
    auto [display_host, display_port] =
        extract_host_and_port(opts.display_address);

    udp::resolver udp_resolver(io_context);
    display_endpoint = *udp_resolver.resolve(display_host, display_port,
                                             resolver_base::numeric_service);

    auto [server_host, server_port] = extract_host_and_port(opts.server_address);
    tcp::resolver tcp_resolver(io_context);

    server_endpoint = *tcp_resolver.resolve(server_host, server_port,
                                            resolver_base::numeric_service);
    boost::asio::ip::tcp::no_delay option(true);
    tcp_server_sock.set_option(option);

    tcp_server_sock.connect(server_endpoint);
    udp_display_sock.connect(display_endpoint);
    // end socket config
  };
};

#endif  // SIK_ZAD3_CLIENT_H
