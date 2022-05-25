#ifndef SIK_ZAD3_CLIENT_H
#define SIK_ZAD3_CLIENT_H

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

#include "Buffer.h"
#include "ByteStream.h"
#include "ClientSerialization.h"
#include "ClientState.h"
#include "common.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

/**
 * Class that handles basic Client functionalities.
 * It asynchronously reads from two associated sockets and
 * acts accordingly to the message.
 */
class Client {
 private:
  std::shared_ptr<udp::socket> udp_display_sock;
  ByteStream udp_stream;
  std::shared_ptr<tcp::socket> tcp_server_sock;
  ByteStream tcp_stream;
  std::string name;
  ClientState aggregated_state;

  /**
   * Function that starts asynchronously listening for UDP messages.
   */
  void udp_start_receive() {
    udp_display_sock->async_wait(
        udp::socket::wait_read,
        boost::bind(&Client::display_msg_rcv_handler, this,
                    boost::asio::placeholders::error));
  }

  /**
   * Method for handling messages from a GUI.
   * First check if boost didn't log any errors, then
   * prepares an appropriate buffer wrapper and tries to deserialize
   * a message.
   * On success sends a matching message to the server.
   * If an exception is thrown during any of the steps, it is caught
   * here, it's message is written to cerr and sockets get closed.
   * @param error any error logged by boost
   */
  void display_msg_rcv_handler(const boost::system::error_code& error) {
    if (error) {
      std::cerr << "Boost error: " << error << std::endl;
      udp_display_sock->close();
      tcp_server_sock->close();
      exit(1);
    }

    try {
      udp_stream.get();
      udp_stream.reset();
      std::shared_ptr<InputMessage> rec_message;

      /* to ignore invalid GUI messages */
      try {
        rec_message = InputMessage::unserialize(udp_stream);
        udp_stream.endRec();
      } catch (std::exception& e) {
        udp_start_receive();
        return;
      }

      tcp_stream.reset();
      if (!aggregated_state.game_on) {
        Join(name).serialize(tcp_stream);
      } else {
        rec_message->serialize(tcp_stream);
      }
      tcp_stream.endWrite();
      udp_start_receive();
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      udp_display_sock->close();
      tcp_server_sock->close();
      exit(1);
    }
  }

  /**
   * Function that starts asynchronously listening for TCP messages.
   */
  void tcp_start_receive() {
    tcp_server_sock->async_wait(tcp::socket::wait_read,
                                boost::bind(&Client::tcp_msg_rcv_handler, this,
                                            boost::asio::placeholders::error));
  }

  /**
   * Method for handling messages from the server.
   * First check if boost didn't log any errors, then
   * prepares an appropriate buffer wrapper and tries to deserialize
   * a message.
   * After that, the message is used to update client state and an
   * appropriate (if any) message to be sent to GUI is prepared and sent.
   * @param error any error logged by boost
   */
  void tcp_msg_rcv_handler(const boost::system::error_code& error) {
    if (error) {
      std::cerr << "Boost error: " << error << std::endl;
      exit(1);
    }
    try {
      tcp_stream.reset();
      std::shared_ptr<ServerMessage> rec_message =
          ServerMessage::unserialize(tcp_stream);

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
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      udp_display_sock->close();
      tcp_server_sock->close();
      exit(1);
    }
  }

 public:
  /**
   * This constructor initializes all fields. To the tcp_stream we pass
   * only a ready socket. It is possible, because tcp is connection based,
   * we can prepare it here and ease the workload for the other class.
   * For udp it's different since we are supposed to be able to read messages
   * from any GUI - we can't connect it.
   */
  Client(boost::asio::io_context& io_context, const ClientCommandLineOpts& opts)
      : udp_display_sock(std::make_shared<udp::socket>(
            io_context, udp::endpoint(udp::v6(), opts.port))),
        udp_stream(std::make_unique<UdpStreamBuffer>(
            udp_display_sock, opts.display_address, io_context)),
        tcp_server_sock(std::make_shared<tcp::socket>(
            io_context, tcp::endpoint(tcp::v6(), opts.port))),
        tcp_stream(std::make_unique<TcpStreamBuffer>(tcp_server_sock)),
        name(opts.player_name) {
    // finding server endpoint
    auto [server_host, server_port] =
        extract_host_and_port(opts.server_address);
    tcp::resolver tcp_resolver(io_context);

    tcp::endpoint server_endpoint = *tcp_resolver.resolve(
        tcp::v6(), server_host, server_port,
        resolver_base::numeric_service | resolver_base::v4_mapped |
            resolver_base::all_matching);

    boost::asio::ip::tcp::no_delay option(true);
    tcp_server_sock->set_option(option);

    tcp_server_sock->connect(server_endpoint);

    tcp_start_receive();
    udp_start_receive();
  };
};

#endif  // SIK_ZAD3_CLIENT_H
