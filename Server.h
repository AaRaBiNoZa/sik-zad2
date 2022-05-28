#ifndef SIK_ZAD2_SERVER_H
#define SIK_ZAD2_SERVER_H

#include <barrier>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include "Message.h"
#include "ServerState.h"
#include "utils.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;

class PlayerConnection {
 private:
  std::shared_ptr<tcp::socket> socket;
  ByteStream tcp_stream;
  std::shared_ptr<ServerState> server_state;
  std::optional<PlayerId> my_id;
  std::shared_ptr<std::barrier<>> game_start_barrier;

 private:
  void init_playing() {
    game_start_barrier->arrive();
    std::cerr << "STARTED PLAYING LULZ";
    start_playing();
  }

  void start_playing() {
    for (;;) {
      tcp_stream.reset();
      std::shared_ptr<ClientMessage> rec_message =
          ClientMessage::unserialize(tcp_stream);

      rec_message->say_hello();
      if (rec_message->TryJoin(*server_state, my_id)) {
        break;
      }
    }
    init_playing();
  }

 public:
  void start_receive() {
    for (;;) {
      tcp_stream.reset();
      std::shared_ptr<ClientMessage> rec_message =
          ClientMessage::unserialize(tcp_stream);

      rec_message->say_hello();
      if (rec_message->TryJoin(*server_state, my_id)) {
        break;
      }
    }
    init_playing();
  }

  void sendHello() {
    tcp_stream.reset();
    Hello(*server_state).serialize(tcp_stream);
    tcp_stream.endWrite();
  }

  std::shared_ptr<tcp::socket> getSocket() {
    return socket;
  }

  explicit PlayerConnection(boost::asio::io_context& io,
                            std::shared_ptr<ServerState> state,
                            std::shared_ptr<std::barrier<>> game_start_barrier,
                            std::shared_ptr<tcp::socket> sock)
      : socket(sock),
        tcp_stream(std::make_unique<TcpStreamBuffer>(socket)),
        server_state(std::move(state)),
        game_start_barrier(std::move(game_start_barrier)){
    boost::asio::ip::tcp::no_delay option(true);
    socket->set_option(option);
        };
};

// will create new connection

class Connector {
 private:
  boost::asio::io_context& io_context;
  tcp::acceptor acceptor;
  std::vector<std::jthread> threads;
  std::shared_ptr<ServerState> state;
  std::vector<std::shared_ptr<PlayerConnection>> connections;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::mutex connections_mutex;
  ByteStream tcp_send_hello_stream;

  void start_accept() {
    std::shared_ptr<tcp::socket> new_socket =
        std::make_shared<tcp::socket>(io_context);

    acceptor.async_accept(
        *new_socket, boost::bind(&Connector::connectionHandler, this,
                                 new_socket, boost::asio::placeholders::error));
  }

  void connectionHandler(std::shared_ptr<tcp::socket> sock,
                         const boost::system::error_code& error) {
    std::cerr << "Got a new connection lulz \n";
    std::shared_ptr<PlayerConnection> new_connection =
        std::make_shared<PlayerConnection>(io_context, state,
                                           game_start_barrier, sock);

    new_connection->sendHello();

    std::unique_lock lk(connections_mutex);
    connections.push_back(new_connection);
    threads.emplace_back(&PlayerConnection::start_receive, new_connection);
    start_accept();
  }

 public:
  static void init(const std::shared_ptr<Connector>& connector) {
    connector->start_accept();
    connector->io_context.run();
  }
  Connector(boost::asio::io_context& io_context,
            const ServerCommandLineOpts& opts,
            std::shared_ptr<ServerState> state,
            std::shared_ptr<std::barrier<>> game_start_barrier)
      : io_context(io_context),
        acceptor(io_context, tcp::endpoint(tcp::v6(), opts.port)),
        state(std::move(state)),
        game_start_barrier(std::move(game_start_barrier)),
        tcp_send_hello_stream(std::make_unique<TcpStreamBuffer>()){

        };
};

class Server {
 private:
  std::shared_ptr<ServerState> server_state;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::shared_ptr<Connector> connector;
  std::jthread connector_thread;

 public:
  void start_game() {
    game_start_barrier->arrive_and_wait();
    std::cerr << "GAME STARTED";
  }
  void init() {
    start_game();
  }

//  void do_one_turn

  Server(boost::asio::io_context& io_context, ServerCommandLineOpts opts)
      : server_state(std::make_shared<ServerState>(opts)),
        game_start_barrier(
            std::make_shared<std::barrier<>>(opts.players_count + 1)),
        connector(std::make_shared<Connector>(io_context, opts, server_state,
                                              game_start_barrier)) {
    connector_thread = std::jthread(&Connector::init, connector);
    init();
  };
};

#endif  // SIK_ZAD2_SERVER_H
