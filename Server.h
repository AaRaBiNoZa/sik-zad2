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
  ByteStream tcp_receive_stream;
  ByteStream tcp_send_stream;
  std::shared_ptr<ServerState> server_state;
  std::optional<PlayerId> my_id;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::mutex send_mutex;
  std::condition_variable_any wait_for_get_turns;
  std::condition_variable_any add_my_mess;

 private:
  void init_playing() {
    game_start_barrier->arrive_and_wait();
    std::cerr << "STARTED PLAYING LULZ";
    start_playing();
  }

  void start_playing() {
    for (;;) {
      tcp_receive_stream.reset();
      std::shared_ptr<ClientMessage> rec_message =
          ClientMessage::unserialize(tcp_receive_stream);
      std::shared_lock lk(server_state->get_client_messages_rw());
      add_my_mess.wait(lk, [&]{return server_state->get_want_to_write_to_rcv_events() == 0;});
      server_state->getMessagesFromTurn()[*my_id] = rec_message;
      rec_message->say_hello();
    }
  }

  void start_observing() {

  }

 public:
  void start_receive() {
    for (;;) {
      if (server_state->get_game_started()) {
        start_observing();
      }
      tcp_receive_stream.reset();
      std::shared_ptr<ClientMessage> rec_message =
          ClientMessage::unserialize(tcp_receive_stream);

      rec_message->say_hello();
      std::stringstream endpoint_string;
      endpoint_string << socket->remote_endpoint();
      if (rec_message->TryJoin(*server_state, my_id, endpoint_string.str())) {
        break;
      }
    }
    init_playing();
  }
// first block connections, then do stuff with join

  void sendInitMessage() {
    std::lock_guard lk(send_mutex);
    tcp_send_stream.reset();
    Hello(*server_state).serialize(tcp_send_stream);
    if (server_state->get_game_started()) {
      std::shared_lock lock2(server_state->get_turn_vec_mut());
      wait_for_get_turns.wait(lock2, [&]{return server_state->get_want_to_write_to_turns() == 0;});
      for (auto k: server_state->get_all_turns()) {
        k->serialize(tcp_send_stream);
      }
    } else {
      std::map<PlayerId, Player> players = server_state->get_players();
      for (auto [id, player]: players) {
        AcceptedPlayer(id, player).serialize(tcp_send_stream);
      }
    }
    tcp_send_stream.endWrite();
  }

  void sendMessage(ServerMessage& msg) {
    std::lock_guard lk(send_mutex);
    tcp_send_stream.reset();
    msg.serialize(tcp_send_stream);
    tcp_send_stream.endWrite();
  }

  void wakeMeNewTurn() {
    add_my_mess.notify_all();
  }

  void wakeMeGetTurn() {
    wait_for_get_turns.notify_all();
  }

  std::shared_ptr<tcp::socket> getSocket() {
    return socket;
  }

  explicit PlayerConnection(boost::asio::io_context& io,
                            std::shared_ptr<ServerState> state,
                            std::shared_ptr<std::barrier<>> game_start_barrier,
                            std::shared_ptr<tcp::socket> sock)
      : socket(sock),
        tcp_receive_stream(std::make_unique<TcpStreamBuffer>(socket)),
        tcp_send_stream(std::make_unique<TcpStreamBuffer>(socket)),
        server_state(std::move(state)),
        game_start_barrier(std::move(game_start_barrier))
        {
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
  std::vector<std::shared_ptr<std::mutex>> connection_send_mutexes;
  ByteStream tcp_send_hello_stream;

  void start_accept() {
    std::shared_ptr<tcp::socket> new_socket =
        std::make_shared<tcp::socket>(io_context);

    acceptor.async_accept(
        *new_socket, boost::bind(&Connector::connectionHandler, this,
                                 new_socket, boost::asio::placeholders::error));
  }

  // needs to be sure that


  void connectionHandler(std::shared_ptr<tcp::socket> sock,
                         const boost::system::error_code& error) {
    std::cerr << "Got a new connection lulz \n";
    std::shared_ptr<PlayerConnection> new_connection =
        std::make_shared<PlayerConnection>(io_context, state,
                                           game_start_barrier, sock);

    //  no new players can be added here
    std::unique_lock players_lock(state->get_players_mut());
    new_connection->sendInitMessage();

    std::unique_lock lk(connections_mutex); // so taht if some other thread would join and we'd broadcast, this one doesn't miss out
    connections.push_back(new_connection);
    lk.unlock();

    // after this, the server will send out broadcast
    players_lock.unlock();

    threads.emplace_back(&PlayerConnection::start_receive, new_connection);
    start_accept();
  }

 public:
  void init() {
    start_accept();
    io_context.run();
  }

  void broadcastAMessage(ServerMessage& msg) {
    std::lock_guard lk(connections_mutex);
    for (auto &connection: connections) {
      connection->sendMessage(msg);
    }
  }

  void wakeAllNewTurn() {
    std::lock_guard lk(connections_mutex);
    for (auto &connection: connections) {
      connection->wakeMeNewTurn();
    }
  }

  void wakeAllGetTurn() {
    std::lock_guard lk(connections_mutex);
    for (auto &connection: connections) {
      connection->wakeMeGetTurn();
    }
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
  boost::asio::io_context& io;

 public:



  void start_lobby() {
    for (uint16_t i = 0; i < server_state->get_players_count(); ++i) {
      game_start_barrier->arrive_and_wait();
      auto player = server_state->getPlayer(i);
      AcceptedPlayer acc_msg = AcceptedPlayer(i, player);
      connector->broadcastAMessage(acc_msg);
    }
    GameStarted game_started_message(*server_state);
    server_state->start_game();
    connector->broadcastAMessage(game_started_message);
    init_game();

  }

  void init_game() {
    std::cerr << "GAME STARTED";
    std::shared_ptr<Turn> init_turn = std::make_shared<Turn>(0);
    for (auto k: server_state->get_players()) {
      Position init_pos(server_state->get_rand().getNextVal() % server_state->get_size_x(), server_state->get_rand().getNextVal() % server_state->get_size_y());
      std::shared_ptr<PlayerMoved> msg = std::make_shared<PlayerMoved>(k.first, init_pos);
      server_state->movePlayer(k.first, init_pos);
      init_turn->addEvent(msg);
    }
    for (uint16_t i = 0; i < server_state->get_initial_blocks(); ++i) {
      Position init_pos(server_state->get_rand().getNextVal() % server_state->get_size_x(), server_state->get_rand().getNextVal() % server_state->get_size_y());
      std::shared_ptr<BlockPlaced> msg = std::make_shared<BlockPlaced>(init_pos);
      server_state->placeBlock(init_pos);
      init_turn->addEvent(msg);
    }
    connector->broadcastAMessage(*init_turn);
    server_state->addTurn(init_turn);

    start_game();
  }

  void start_game() {

    for (uint16_t i = 1; i < server_state->get_game_length() + 1; ++i) {
      boost::asio::steady_timer turn_timer(io, boost::asio::chrono::milliseconds (server_state->get_turn_duration()));
      turn_timer.wait();

      doOneTurn(i);
    }
  }


  void doOneTurn(uint16_t turn_num) {
      server_state->get_want_to_write_to_rcv_events()++;
      std::unique_lock lk(server_state->get_client_messages_rw());
      std::shared_ptr<Turn> cur_turn = std::make_shared<Turn>(turn_num);

      for (auto [id, bomb]: server_state->get_bombs()) {
        auto opt_val = server_state->checkBomb(id);
        if (opt_val) {
          auto [a,b] = *opt_val;
          std::shared_ptr<BombExploded> bomb_explosion = std::make_shared<BombExploded>(id, b,a);
          cur_turn->addEvent(bomb_explosion);
        }

      }

      auto dead_players = server_state->cleanUpBombs();

      for (auto [id, msg]: server_state->getMessagesFromTurn()) {
        if (!dead_players.contains(id) && msg) {
          msg->updateServerState(*server_state, id, cur_turn); // and id, //and turn
          std::cerr << "\n\nREADING NOW\n\n ";
        } else if (dead_players.contains(id)) {
          Position new_pos(server_state->get_rand().getNextVal() % server_state->get_size_x(), server_state->get_rand().getNextVal() % server_state->get_size_y());
          std::shared_ptr<PlayerMoved> msg = std::make_shared<PlayerMoved>(id, new_pos);
          server_state->movePlayer(id, new_pos);
          cur_turn->addEvent(msg);
        }
      }
      server_state->resetmessagesfromturn();
      connector->broadcastAMessage(*cur_turn);
      server_state->addTurn(cur_turn);
      server_state->get_want_to_write_to_rcv_events()--;
      connector->wakeAllNewTurn();
      connector->wakeAllGetTurn();


  }

//  void do_one_turn

  Server(boost::asio::io_context& io_context, ServerCommandLineOpts opts)
      : server_state(std::make_shared<ServerState>(opts)),
        game_start_barrier(
            std::make_shared<std::barrier<>>(2)),
        connector(std::make_shared<Connector>(io_context, opts, server_state,
                                              game_start_barrier)), io(io_context) {
    connector_thread = std::jthread(&Connector::init, connector);
    start_lobby();
  };
};

#endif  // SIK_ZAD2_SERVER_H
