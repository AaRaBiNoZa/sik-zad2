//
// Created by Adam Al-Hosam on 04/05/2022.
//
#ifndef SIK_ZAD3_CLIENTCONFIG_H
#define SIK_ZAD3_CLIENTCONFIG_H

#include <string>
#include "common.h"
#include "boost/asio.hpp"


using boost::asio::ip::tcp;
using boost::asio::ip::udp;
using boost::asio::ip::resolver_base;

class ClientConfig {
 private:
  udp::endpoint display_endpoint;
  tcp::endpoint server_endpoint;
  uint16_t port;
  std::string player_name;


 public:
  ClientConfig(boost::asio::io_context& io_context, std::string &display_address, std::string &server_address,
               uint16_t port, std::string &player_name)
      : port(port), player_name(player_name) {

    auto [display_host, display_port] = extract_host_and_port(display_address);
    auto [server_host, server_port] = extract_host_and_port(display_address);

    udp::resolver udp_resolver(io_context);
    display_endpoint = *udp_resolver.resolve(display_host, display_port, resolver_base::numeric_service);

    tcp::resolver tcp_resolver(io_context);
    server_endpoint = *tcp_resolver.resolve(server_host, server_port, resolver_base::numeric_service);
  }
};

#endif  // SIK_ZAD3_CLIENTCONFIG_H
