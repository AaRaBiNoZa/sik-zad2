//
// Created by Adam Al-Hosam on 04/05/2022.
//

#ifndef SIK_ZAD3_TCPCLIENTCONNECTION_H
#define SIK_ZAD3_TCPCLIENTCONNECTION_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>

#include "common.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;

class TcpClientConnection {
 private:
  tcp::socket socket;
  tcp::endpoint server_endpoint;  // not needed i think
  boost::array<char, 2> buf;
  boost::system::error_code error;

  void start_receive() {
    socket.async_read_some(boost::asio::buffer(buf),
                           boost::bind(&TcpClientConnection::handle_receive,
                                       this, boost::asio::placeholders::error));
  }

  void handle_receive(const boost::system::error_code& error) {
    if (!error) {
      std::cout << buf.data() << "\n";
      boost::shared_ptr<std::string> message(
          new std::string("LOL"));

      socket.async_send(
          boost::asio::buffer(*message),
          boost::bind(&TcpClientConnection::handle_send, this, message,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));

      start_receive();
    } else {
      std::cerr << "Finished with wtcp";
    }
  }

  void handle_send(const boost::shared_ptr<std::string>& /*message*/,
                   const boost::system::error_code&, std::size_t) {
  }
 public:
  TcpClientConnection(boost::asio::io_context& io_context, uint16_t port,
                      std::string server_addr)
      : socket(io_context, tcp::endpoint(tcp::v4(), port)) {

    auto [server_host, server_port] = extract_host_and_port(server_addr);
    tcp::resolver tcp_resolver(io_context);

    server_endpoint = *tcp_resolver.resolve(server_host, server_port,
                                            resolver_base::numeric_service);
    boost::asio::ip::tcp::no_delay option(true);
    socket.set_option(option);

   socket.connect(server_endpoint);


    boost::shared_ptr<std::string> message(
        new std::string("LOL"));

    socket.async_send(
        boost::asio::buffer(*message),
        boost::bind(&TcpClientConnection::handle_send, this, message,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

    start_receive();
  }
};

#endif  // SIK_ZAD3_TCPCLIENTCONNECTION_H
