#ifndef SIK_ZAD2_SERVER_H
#define SIK_ZAD2_SERVER_H

#include <barrier>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include "ServerState.h"
#include "Message.h"
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
  void start_receive() {
    for (;;) {
      tcp_stream.reset();
      std::shared_ptr<ClientMessage> rec_message = ClientMessage::unserialize(tcp_stream);

      rec_message->say_hello();
      if (rec_message->TryJoin(*server_state, my_id)) {
        break;
      }
    }
    start_playing();
  }

  void start_playing() {
    game_start_barrier->arrive_and_wait();
    std::cerr << "STARTED PLAYING LULZ";
    sleep(20000);
  }

 public:
  tcp::socket& getSocket() {
    return *socket;
  }

  static void manageConnection(std::shared_ptr<PlayerConnection>& connection) {
    // do stuff
    boost::asio::ip::tcp::no_delay option(true);
    connection->socket->set_option(option);
    connection->start_receive();
    //    io.run();
  }

  explicit PlayerConnection(boost::asio::io_context& io,
                         std::shared_ptr<ServerState> state,  std::shared_ptr<std::barrier<>> game_start_barrier
                            )
      : socket(std::make_shared<tcp::socket>(io)),
        tcp_stream(std::make_unique<TcpStreamBuffer>(socket)),
        server_state(std::move(state)), game_start_barrier(std::move(game_start_barrier)){

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

  void start_accept() {
    std::shared_ptr<PlayerConnection> connection = std::make_shared<PlayerConnection>(
        io_context, state, game_start_barrier);
    connections.push_back(connection);
    acceptor.async_accept(
        connection->getSocket(),
        boost::bind(&Connector::connectionHandler, this, connection,
                    boost::asio::placeholders::error));
  }

  void connectionHandler(std::shared_ptr<PlayerConnection>& connection,
                         const boost::system::error_code& error) {
    std::cerr << "Got a new connection lulz \n";
    threads.emplace_back(&PlayerConnection::manageConnection,
                         std::ref(connection));
    start_accept();
  }

 public:
  static void init(const std::shared_ptr<Connector>& connector) {
    connector->start_accept();
    connector->io_context.run();
  }
  Connector(boost::asio::io_context& io_context, const ServerCommandLineOpts& opts,
            std::shared_ptr<ServerState> state, std::shared_ptr<std::barrier<>> game_start_barrier)
      : io_context(io_context),
        acceptor(io_context, tcp::endpoint(tcp::v6(), opts.port)),
        state(std::move(state)), game_start_barrier(std::move(game_start_barrier)){};
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
    std::cerr << "Game started";
  }
  void init() {
    start_game();
  }

  Server(boost::asio::io_context& io_context, ServerCommandLineOpts opts)
      : server_state(std::make_shared<ServerState>(opts)),
        game_start_barrier(std::make_shared<std::barrier<>>(opts.players_count + 1)), connector(std::make_shared<Connector>(
            io_context, opts, server_state, game_start_barrier)) {
    connector_thread = std::jthread(&Connector::init, connector);
    init();
  };
};

#endif  // SIK_ZAD2_SERVER_H
