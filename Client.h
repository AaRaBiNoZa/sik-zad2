//
// Created by ciriubuntu on 11.05.22.
//

#ifndef SIK_ZAD3_CLIENT_H
#define SIK_ZAD3_CLIENT_H

//#include "ClientState.h"
#include "TcpClientConnection.h"
#include "Messages.h"
#include <boost/bind/bind.hpp>

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
 private:
  std::shared_ptr<udp::socket> udp_display_sock;
  std::shared_ptr<tcp::socket> tcp_server_sock;
  ByteStream udp_stream;
  ByteStream tcp_stream;
  std::string name;
  ClientState aggregated_state;
  void init() {
    udp_start_receive();
  }

  void udp_start_receive() {
    udp_display_sock->async_wait(udp::socket::wait_read, boost::bind(&Client::display_msg_rcv_handler, this, boost::asio::placeholders::error));
  }

  void display_msg_rcv_handler(const boost::system::error_code& error) {
    udp_stream.get();
    udp_stream.reset();
    std::cerr << "Here";
    std::shared_ptr<InputMessage> rec_message = InputMessage::unserialize(udp_stream);
    udp_stream.endRec();

    tcp_stream.reset();
    if (!aggregated_state.game_on) {
      Join(name).serialize(tcp_stream);
    } else {
      rec_message->serialize(tcp_stream);
    }
    tcp_stream.endWrite();
    udp_start_receive();
  }

  void tcp_start_receive() {
    tcp_server_sock->async_wait(tcp::socket::wait_read, boost::bind(&Client::tcp_msg_rcv_handler, this, boost::asio::placeholders::error));
  }

  void tcp_msg_rcv_handler(const boost::system::error_code& error) {
    tcp_stream.reset();
    std::shared_ptr<ServerMessage> rec_message = ServerMessage::unserialize(tcp_stream);
    tcp_stream.endRec();
    rec_message->say_hello();
    std::cerr << '\n';

    if (rec_message->updateClientState(aggregated_state)) {
      udp_stream.reset();
      if (!aggregated_state.game_on) {
        Lobby(aggregated_state).serialize(udp_stream);
      } else {
        Game(aggregated_state).serialize(udp_stream);
      }
      udp_stream.endWrite();
    }
    tcp_start_receive();
  }

//  void tcp_send_response() {
//
//  }
//  void server_msg_rcv_handler()
//  void display_msg_send_handler()
//  void server_msg_send_handler()
 public:
  Client(boost::asio::io_context& io_context, ClientCommandLineOpts opts)
      : udp_display_sock(std::make_shared<udp::socket>(
            io_context, udp::endpoint(udp::v4(), opts.port))),
        udp_stream(std::make_unique<UdpStreamBuffer>(udp_display_sock)),
        tcp_server_sock(std::make_shared<tcp::socket>(
            io_context, tcp::endpoint(tcp::v4(), opts.port))),
        tcp_stream(std::make_unique<TcpStreamBuffer>(tcp_server_sock)), name(opts.player_name) {
    auto [display_host, display_port] =
        extract_host_and_port(opts.display_address);

    udp::resolver udp_resolver(io_context);
    udp::endpoint display_endpoint = *udp_resolver.resolve(
        display_host, display_port, resolver_base::numeric_service);

    auto [server_host, server_port] = extract_host_and_port(opts.server_address);
    tcp::resolver tcp_resolver(io_context);

    tcp::endpoint server_endpoint = *tcp_resolver.resolve(server_host, server_port,
                                            resolver_base::numeric_service);
    boost::asio::ip::tcp::no_delay option(true);
    tcp_server_sock->set_option(option);

    udp_display_sock->connect(display_endpoint);
    tcp_server_sock->connect(server_endpoint);
    udp_start_receive();
    tcp_start_receive();
  };
};

#endif  // SIK_ZAD3_CLIENT_H
