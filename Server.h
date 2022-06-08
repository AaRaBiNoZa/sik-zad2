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
  std::optional<std::shared_ptr<ClientMessage>> last_msg;

  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::mutex send_mutex;

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
          ClientMessage::deserialize(tcp_receive_stream);

      if (rec_message->amIJoin()) {
        continue;
      }
      std::shared_lock client_message_lock(server_state->getClientMessagesRw());
      server_state->getWaitForPlayersMessages().wait(client_message_lock, [&] {
        return server_state->getWantToWriteToClientMessages() == 0;
      });

      if (my_id) {
        server_state->getMessagesFromTurnNoSync()[*my_id] = rec_message;
      } else {
        last_msg.emplace(rec_message);
        return;
      }
      rec_message->say_hello();
    }
  }

 public:
  void start_receive() {
    try {
      for (;;) {
        std::shared_ptr<ClientMessage> rec_message;
        if (!last_msg) {
          tcp_receive_stream.reset();
          rec_message = ClientMessage::deserialize(tcp_receive_stream);
        } else {
          rec_message = *last_msg;
          last_msg.reset();
        }

        rec_message->say_hello();
        std::stringstream endpoint_string;
        endpoint_string << socket->remote_endpoint();
        if (!server_state->getGameStarted() &&
            rec_message->TryJoin(*server_state, my_id, endpoint_string.str())) {
          init_playing();
        }
      }
    }
    catch (std::exception& e) {
      socket->close();
      return;
    }
  }
  // first block connections, then do stuff with join

  // this is only ever called by the connector,
  // here we players are locked with shared access and connections with
  // exclusive
  void sendInitMessage() {
    std::lock_guard lk(send_mutex);
    tcp_send_stream.reset();
    Hello(*server_state).serialize(tcp_send_stream);
    tcp_send_stream.endWrite();

    tcp_send_stream.reset();
    if (server_state->getGameStarted()) {  // GAME STARTED MSG
      std::shared_lock players_lock(
          server_state
              ->getPlayersMutex());  // noone is allowed to join, because we
                                     // want to send initial/broadcast message
      server_state->getWaitForPlayers().wait(players_lock, [&] {
        return server_state->getWantToWriteToPlayers() == 0;
      });

      GameStarted game_started_msg(server_state->getPlayers());
      game_started_msg.serialize(tcp_send_stream);
      tcp_send_stream.endWrite();

      tcp_send_stream.reset();

      // getting shared access to turns (reader)
      std::shared_lock turns_lock(server_state->getAllTurnsMutex());
      server_state->getWaitForTurns().wait(turns_lock, [&] {
        return server_state->getWantToWriteToTurns() == 0;
      });

      for (auto k : server_state->getAllTurnsNoSync()) {
        k->serialize(tcp_send_stream);
      }

      tcp_send_stream.endWrite();
    } else {


      std::map<PlayerId, Player> players = server_state->getPlayers();
      for (auto [id, player] : players) {
        AcceptedPlayer(id, player).serialize(tcp_send_stream);
      }
      tcp_send_stream.endWrite();
    }
  }

  void sendMessage(ServerMessage& msg) {
    std::lock_guard lk(send_mutex);
    tcp_send_stream.reset();
    msg.serialize(tcp_send_stream);
    tcp_send_stream.endWrite();
  }

  void endPlaying() {
    my_id.reset();
  }

  explicit PlayerConnection(std::shared_ptr<ServerState> state,
                            std::shared_ptr<std::barrier<>> game_start_barrier,
                            std::shared_ptr<tcp::socket> sock)
      : socket(sock),
        tcp_receive_stream(std::make_unique<TcpStreamBuffer>(socket)),
        tcp_send_stream(std::make_unique<TcpStreamBuffer>(socket)),
        server_state(std::move(state)),
        game_start_barrier(std::move(game_start_barrier)) {
    boost::asio::ip::tcp::no_delay option(true);
    socket->set_option(option);
  };
};

// will create new connection

class Connector {
 private:
  boost::asio::io_context& io_context;
  tcp::acceptor acceptor;
  std::shared_ptr<ServerState> state;
  std::set<std::shared_ptr<PlayerConnection>> connections;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::mutex connections_mutex;

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
        std::make_shared<PlayerConnection>(state, game_start_barrier, sock);

    std::unique_lock lk(
        connections_mutex);  // noone is allowed to broadcast, since this
                             // connection wouldn't be included

    try {
      new_connection->sendInitMessage();
    } catch (std::exception& e){
      start_accept();
      return;
    }

    connections.insert(new_connection);
    lk.unlock();

    std::jthread(&PlayerConnection::start_receive, new_connection).detach();
    start_accept();
  }

 public:
  void init() {
    start_accept();
    io_context.run();
  }

  void broadcastAMessage(ServerMessage& msg) {
    std::lock_guard lk(connections_mutex);
    std::set<std::shared_ptr<PlayerConnection> >to_delete;
    for (auto& connection : connections) {
      try {
        connection->sendMessage(msg);
      } catch (std::exception& e){
        to_delete.insert(connection);
      }
    }
    for (auto &connection: to_delete) {
      connections.erase(connection);
    }
  }

  void finish() {
    std::lock_guard lk(connections_mutex);
    std::set<std::shared_ptr<PlayerConnection> >to_delete;
    for (auto& connection : connections) {
      try {
        connection->endPlaying();
      } catch (std::exception& e) {
        to_delete.insert(connection);
      }
    }

    for (auto &connection : to_delete) {
      connections.erase(connection);
    }
  }

  Connector(boost::asio::io_context& io_context,
            const ServerCommandLineOpts& opts,
            std::shared_ptr<ServerState> state,
            std::shared_ptr<std::barrier<>> game_start_barrier)
      : io_context(io_context),
        acceptor(io_context, tcp::endpoint(tcp::v6(), opts.port)),
        state(std::move(state)),
        game_start_barrier(std::move(game_start_barrier)){

        };
};

class Server {
 private:
  std::shared_ptr<ServerState> server_state;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::shared_ptr<Connector> connector;
  std::jthread connector_thread;
  boost::asio::steady_timer turn_timer;

 public:
  void start_lobby() {
    for (uint16_t i = 0; i < server_state->getPlayersCount(); ++i) {
      game_start_barrier->arrive_and_wait();

      auto player = server_state->getPlayerSync(
          i);  // safe read - at this moment this slot
                                       // ought to be valid and written to

      AcceptedPlayer acc_msg = AcceptedPlayer(i, player);
      connector->broadcastAMessage(
          acc_msg);  // this is responsibility of connector to synchronize
                     // correctly
    }
  // add mutex to players
    GameStarted game_started_message(*server_state);
    server_state->startGame();  // atomic
    connector->broadcastAMessage(
        game_started_message);  // connector's responsibility

    init_game();
  }

  // players don't interfere with server state directly during the game, all
  // interpretations of their messages is done here, in this class
  void init_game() {
    std::cerr << "GAME STARTED";
    std::shared_ptr<Turn> init_turn = std::make_shared<Turn>(
        0);  // preparations for the game, initial positions etc

    for (auto [playerId, player] : server_state->getPlayers()) {
      Position init_pos(
          server_state->getRand().getNextVal() % server_state->getSizeX(),
          server_state->getRand().getNextVal() % server_state->getSizeY());
      std::shared_ptr<PlayerMoved> msg =
          std::make_shared<PlayerMoved>(playerId, init_pos);
      server_state->movePlayer(playerId, init_pos);
      init_turn->addEvent(msg);
    }

    for (uint16_t i = 0; i < server_state->getInitialBlocks(); ++i) {
      Position init_pos(
          server_state->getRand().getNextVal() % server_state->getSizeX(),
          server_state->getRand().getNextVal() % server_state->getSizeY());
      std::shared_ptr<BlockPlaced> msg =
          std::make_shared<BlockPlaced>(init_pos);
      server_state->placeBlock(init_pos);
      init_turn->addEvent(msg);
    }
    connector->broadcastAMessage(*init_turn);
    server_state->addTurnSync(init_turn);

    start_game();
  }

  void start_game() {
    for (uint16_t i = 1; i < server_state->getGameLength() + 1; ++i) {
      turn_timer.expires_after(
          boost::asio::chrono::milliseconds(server_state->getTurnDuration()));
      turn_timer.wait();
      doOneTurn(i);
    }
    turn_timer.expires_after(
        boost::asio::chrono::milliseconds(server_state->getTurnDuration()));
    turn_timer.wait();
    endGame();
  }

  void endGame() {
    server_state->getWantToWriteToClientMessages()++;  // blocking all that got any messages
    std::unique_lock lk(server_state->getClientMessagesMutex());
    server_state->resetMessagesFromPlayersNoSync();
    connector->finish();

    server_state->reset();
    server_state->getWantToWriteToClientMessages()--;
    server_state->wakeWaitingForSharedClientMessages();

    auto new_msg = GameEnded(server_state->getScores());
    connector->broadcastAMessage(new_msg);
  }

  void doOneTurn(uint16_t turn_num) {
    server_state->getWantToWriteToClientMessages()++;
    std::unique_lock lk(
        server_state
            ->getClientMessagesMutex());  // blocking saving recent messages, since the turn has ended
    std::shared_ptr<Turn> cur_turn = std::make_shared<Turn>(turn_num);

    for (auto [id, bomb] : server_state->getBombs()) {
      auto opt_val = server_state->checkBomb(id);
      if (opt_val) {
        auto [a, b] = *opt_val;
        std::shared_ptr<BombExploded> bomb_explosion =
            std::make_shared<BombExploded>(id, b, a);
        cur_turn->addEvent(bomb_explosion);
      }
    }

    auto dead_players = server_state->cleanUpBombs();

    for (auto [id, msg] : server_state->getMessagesFromTurnNoSync()) {
      if (!dead_players.contains(id) && msg) {
        msg->updateServerState(*server_state, id,
                               cur_turn);  // and id, //and turn
        std::cerr << "\n\nREADING NOW\n\n ";
      } else if (dead_players.contains(id)) {
        Position new_pos(
            server_state->getRand().getNextVal() % server_state->getSizeX(),
            server_state->getRand().getNextVal() % server_state->getSizeY());
        std::shared_ptr<PlayerMoved> new_msg =
            std::make_shared<PlayerMoved>(id, new_pos);
        server_state->movePlayer(id, new_pos);
        cur_turn->addEvent(new_msg);
      }
    }
    server_state->resetMessagesFromPlayersNoSync();
    connector->broadcastAMessage(*cur_turn);
    server_state->addTurnSync(cur_turn);
    server_state->getWantToWriteToClientMessages()--;
    server_state->wakeWaitingForSharedClientMessages();
  }

  //  void do_one_turn

  Server(boost::asio::io_context& io_context, ServerCommandLineOpts opts)
      : server_state(std::make_shared<ServerState>(opts)),
        game_start_barrier(std::make_shared<std::barrier<>>(2)),
        connector(std::make_shared<Connector>(io_context, opts, server_state,
                                              game_start_barrier)),
        turn_timer(io_context) {
    connector_thread = std::jthread(&Connector::init, connector);

    for (;;) {
      start_lobby();
    }
  };
};

#endif  // SIK_ZAD2_SERVER_H
