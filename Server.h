//
// Created by ciriubuntu on 14.05.22.
//

#ifndef SIK_ZAD3_SERVER_H
#define SIK_ZAD3_SERVER_H

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>
#include <utility>
#include <vector>
#include <barrier>

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;


class TcpConnection {
 private:
  std::shared_ptr<tcp::socket> socket;
  ByteStream tcp_stream;
  std::shared_ptr<ServerState> server_state;
  std::optional<PlayerId> my_id;
  std::shared_ptr<std::barrier<>> game_start_barrier;

 public:
  void start_receive() {
    socket->async_wait(tcp::socket::wait_read, boost::bind(&TcpConnection::rcv_handler, this, boost::asio::placeholders::error));
  }

  void start_playing() {
    std::cerr << "Connection started playing\n";
    sleep(20);
  }

  void rcv_handler(const boost::system::error_code& error) {
    tcp_stream.reset();
    // check if done
    std::shared_ptr<ClientMessage> rec_message =
        ClientMessage::unserialize(tcp_stream);
    tcp_stream.endRec();
    rec_message->say_hello();
    rec_message->updateServerState(*server_state, my_id);
    if (my_id) {
      game_start_barrier->arrive();
      start_playing();
    } else {
      start_receive();
    }
  }

  void sent(const boost::system::error_code& error) {
    std::cerr << "Send handler \n";
  }

  static void manageConnection(boost::asio::io_context& io,
                               std::shared_ptr<TcpConnection>& connection) {
    // do stuff
    boost::asio::ip::tcp::no_delay option(true);
    connection->socket->set_option(option);
    connection->start_receive();
    io.run();
  }

  std::shared_ptr<tcp::socket> getSocket() {
    return socket;
  }
  explicit TcpConnection(boost::asio::io_context& io, std::shared_ptr<ServerState> state, std::shared_ptr<std::barrier<>> barrier)
      : socket(std::make_shared<tcp::socket>(io)),
        tcp_stream(std::make_unique<TcpStreamBuffer>(socket)), server_state(state), game_start_barrier(barrier) {

        };

};

// will create new connection

class Connector {
 private:
  boost::asio::io_context& io_context;
  tcp::acceptor acceptor;
  std::vector<std::jthread> threads;
  std::shared_ptr<ServerState> state;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::vector<std::shared_ptr<TcpConnection>> connections;

  void start_accept() {
    std::shared_ptr<TcpConnection> connection =
        std::make_shared<TcpConnection>(io_context, state, game_start_barrier);
    connections.push_back(connection);
    acceptor.async_accept(
        *connection->getSocket(),
        boost::bind(&Connector::connectionHandler, this, connection,
                    boost::asio::placeholders::error));
  }

  void connectionHandler(std::shared_ptr<TcpConnection>& connection,
                         const boost::system::error_code& error) {
    std::cerr << "Got a new connection lulz \n";
    threads.emplace_back(&TcpConnection::manageConnection, std::ref(io_context),
                   std::ref(connection));
    start_accept();
  }

 public:
  static void init(std::shared_ptr<Connector> connector) {
    connector->start_accept();
    connector->io_context.run();
  }
  Connector(boost::asio::io_context& io_context, ServerCommandLineOpts opts,
            std::shared_ptr<ServerState> state, std::shared_ptr<std::barrier<>> barrier)
      : io_context(io_context),
        acceptor(io_context, tcp::endpoint(tcp::v4(), opts.port)),
        state(state), game_start_barrier(barrier){};
};

class Server {
 private:
  std::shared_ptr<ServerState> server_state;
  std::shared_ptr<std::barrier<>> game_start;
  std::shared_ptr<Connector> connector;
  std::jthread jthread;

 public:
  void start_game() {
    std::cerr << "Game started";
  }
  void init() {
      game_start->arrive_and_wait();
      start_game();
  }
  Server(boost::asio::io_context& io_context, ServerCommandLineOpts opts)
      : server_state(std::make_shared<ServerState>(opts)),game_start(std::make_shared<std::barrier<>>(server_state->getMaxCount() + 1)),
        connector(std::make_shared<Connector>(io_context, opts, server_state, game_start))  {
    jthread = std::jthread(&Connector::init, connector);
    init();
  };

};

#endif  // SIK_ZAD3_SERVER_H
