//
// Created by ciriubuntu on 14.05.22.
//

#ifndef SIK_ZAD3_SERVER_H
#define SIK_ZAD3_SERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <utility>
#include <vector>

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;



class TcpConnection {
 private:
  std::shared_ptr<tcp::socket> socket;
  ByteStream tcp_stream;

 public:
  void start_receive() {
    socket->async_wait(tcp::socket::wait_read,
                       boost::bind(&TcpConnection::rcv_handler, this,
                                   boost::asio::placeholders::error));
  }

  void rcv_handler(const boost::system::error_code& error) {
    tcp_stream.reset();
    // check if done
    std::shared_ptr<ClientMessage> rec_message =
        ClientMessage::unserialize(tcp_stream);
    tcp_stream.endRec();
    rec_message->say_hello();
    start_receive();
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
  explicit TcpConnection(boost::asio::io_context& io)
      : socket(std::make_shared<tcp::socket>(io)),
        tcp_stream(std::make_unique<TcpStreamBuffer>(socket)){

        };
  void run() {
    return;
  }
};

// will create new connection

class Connector {
 private:
  boost::asio::io_context& io_context;
  tcp::acceptor acceptor;
  std::vector<std::thread> threads;
  std::shared_ptr<ServerState> state;

  void start_accept() {
    std::shared_ptr<TcpConnection> connection =
        std::make_shared<TcpConnection>(io_context);
    acceptor.async_accept(
        *connection->getSocket(),
        boost::bind(&Connector::connectionHandler, this, connection,
                    boost::asio::placeholders::error));
  }

  void connectionHandler(std::shared_ptr<TcpConnection>& connection,
                         const boost::system::error_code& error) {
    std::cerr << "Got a new connection lulz \n";
    std::jthread t(&TcpConnection::manageConnection, std::ref(io_context),
                   std::ref(connection));
    start_accept();
  }

 public:
  static void init(std::shared_ptr<Connector> connector) {
    connector->start_accept();
    connector->io_context.run();
  }
  Connector(boost::asio::io_context& io_context, ServerCommandLineOpts opts,
            std::shared_ptr<ServerState> state)
      : io_context(io_context),
        acceptor(io_context, tcp::endpoint(tcp::v4(), opts.port)),
        state(state){};
};

class Server {
 private:
  std::shared_ptr<ServerState> server_state;
  std::shared_ptr<Connector> connector;

 public:
  Server(boost::asio::io_context& io_context, ServerCommandLineOpts opts)
      : server_state(std::make_shared<ServerState>(opts)),
        connector(std::make_shared<Connector>(io_context, opts, server_state)) {
    std::jthread t(&Connector::init, connector);
  };
};

#endif  // SIK_ZAD3_SERVER_H
