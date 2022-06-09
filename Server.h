#ifndef SIK_ZAD2_SERVER_H
#define SIK_ZAD2_SERVER_H

#include <barrier>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include "ConnectionUtils.h"
#include "Message.h"
#include "MessageUtils.h"
#include "ServerState.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::tcp;

/**
 * This class represents the server's brains. It creates one thread
 * for the server logic. one thread for accepting new connections, and another
 * for receiving messages from players.
 */

/**
 * Represents a connection with the player, main responsibilities are
 * receiving the message and passing it to a shared vector of players messages
 */
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
  void start_playing() {
    game_start_barrier->arrive_and_wait();
    for (;;) {
      tcp_receive_stream.reset();
      std::shared_ptr<ClientMessage> received_message =
          ClientMessage::deserialize(tcp_receive_stream);

      if (received_message->am_i_join()) {
        continue;
      }

      std::shared_lock client_message_lock(
          server_state->get_client_messages_rw());
      server_state->get_wait_for_players_messages().wait(
          client_message_lock, [&] {
            return server_state->get_want_to_write_to_client_messages() == 0;
          });

      if (my_id) {
        server_state->get_messages_from_turn_no_sync()[*my_id] =
            received_message;
      } else {
        last_msg.emplace(received_message);
        return;
      }
    }
  }

 public:
  /*
   * Listens to incoming messages, if player wants to join and they can join,
   * then init_playing is notified and game_start_barrier is notified
   * (so the server knows how many players are already ready)
   */
  void start_receive() {
    try {
      for (;;) {
        std::shared_ptr<ClientMessage> rec_message;

        /** last_msg is here if a player wan't notified, that the game has ended
         * If the game has ended and the player (actually this thread)
         * didn't know about it, because it was blocked, it stores the message
         * it got, processes that the game has ended and only then acts
         * on the message it got
         */
        if (!last_msg) {
          tcp_receive_stream.reset();
          rec_message = ClientMessage::deserialize(tcp_receive_stream);
        } else {
          rec_message = *last_msg;
          last_msg.reset();
        }

        std::stringstream endpoint_string;
        endpoint_string << socket->remote_endpoint();

        if (!server_state->get_game_started() &&
            rec_message->try_join(*server_state, my_id,
                                  endpoint_string.str())) {
          start_playing();
        }
      }
    } catch (std::exception& e) {
      socket->close();
      return;
    }
  }

  /*
   * A proper level of synchronization is needed here,
   * we block any players from joining and from connecting.
   * Accept a player and send them an initial message.
   */
  void send_init_message() {
    std::lock_guard lk(send_mutex);
    tcp_send_stream.reset();
    Hello(*server_state).serialize(tcp_send_stream);
    tcp_send_stream.end_write();

    tcp_send_stream.reset();
    if (server_state->get_game_started()) {
      std::shared_lock players_lock(
          server_state
              ->get_players_mutex());  // noone is allowed to join, because we
                                       // want to send initial/broadcast message
      server_state->get_wait_for_players().wait(players_lock, [&] {
        return server_state->get_want_to_write_to_players() == 0;
      });

      GameStarted game_started_msg(server_state->get_players());
      game_started_msg.serialize(tcp_send_stream);
      tcp_send_stream.end_write();
      tcp_send_stream.reset();

      std::shared_lock turns_lock(server_state->get_all_turns_mutex());
      server_state->get_wait_for_turns().wait(turns_lock, [&] {
        return server_state->get_want_to_write_to_turns() == 0;
      });

      for (auto k : server_state->get_all_turns_no_sync()) {
        k->serialize(tcp_send_stream);
      }

      tcp_send_stream.end_write();
    } else {
      std::map<PlayerId, Player> players = server_state->get_players();
      for (auto [id, player] : players) {
        AcceptedPlayer(id, player).serialize(tcp_send_stream);
      }
      tcp_send_stream.end_write();
    }
  }

  void send_message(ServerMessage& msg) {
    std::lock_guard lk(send_mutex);
    tcp_send_stream.reset();
    msg.serialize(tcp_send_stream);
    tcp_send_stream.end_write();
  }

  void end_playing() {
    my_id.reset();
  }

  explicit PlayerConnection(std::shared_ptr<ServerState> state,
                            std::shared_ptr<std::barrier<>> game_start_barrier,
                            std::shared_ptr<tcp::socket> sock)
      : socket(std::move(sock)),
        tcp_receive_stream(std::make_unique<TcpStreamBuffer>(socket)),
        tcp_send_stream(std::make_unique<TcpStreamBuffer>(socket)),
        server_state(std::move(state)),
        game_start_barrier(std::move(game_start_barrier)) {
    boost::asio::ip::tcp::no_delay option(true);
    socket->set_option(option);
  };
};

/*
 * This class main responsibility is accepting new players and
 * take care of sending initial message.
 */
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
        *new_socket, boost::bind(&Connector::connection_handler, this,
                                 new_socket, boost::asio::placeholders::error));
  }

  void connection_handler(
      std::shared_ptr<tcp::socket> sock,
      [[maybe_unused]] const boost::system::error_code& error) {
    std::cerr << "Got a new connection lulz \n";
    std::shared_ptr<PlayerConnection> new_connection =
        std::make_shared<PlayerConnection>(state, game_start_barrier, sock);

    /* this is done to ensure that noone will broadcast now */
    std::unique_lock lk(connections_mutex);

    try {
      new_connection->send_init_message();
    } catch (std::exception& e) {
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

  /**
   * Will be called by the server to send out the same message to every
   * client.
   * In addition - if something fails during send, server removes this player.
   */
  void broadcast_message(ServerMessage& msg) {
    std::lock_guard lk(connections_mutex);
    std::set<std::shared_ptr<PlayerConnection>> to_delete;
    for (auto& connection : connections) {
      try {
        connection->send_message(msg);
      } catch (std::exception& e) {
        to_delete.insert(connection);
      }
    }
    for (auto& connection : to_delete) {
      connections.erase(connection);
    }
  }

  /**
   * Called when the game ends, notifies each player about this fact +
   * does additional checking if it should remove any players.
   */
  void finish() {
    std::lock_guard lk(connections_mutex);
    std::set<std::shared_ptr<PlayerConnection>> to_delete;
    for (auto& connection : connections) {
      try {
        connection->end_playing();
      } catch (std::exception& e) {
        to_delete.insert(connection);
      }
    }

    for (auto& connection : to_delete) {
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

/*
 * This class first waits for an appropriate number of players to join.
 * Then it initializes the game state and sends appropriate messages.
 */
class Server {
 private:
  std::shared_ptr<ServerState> server_state;
  std::shared_ptr<std::barrier<>> game_start_barrier;
  std::shared_ptr<Connector> connector;
  std::jthread connector_thread;
  boost::asio::steady_timer turn_timer;

  /*
   * Here the waiting is done, after each player joins, this thread
   * broadcasts an appropriate message.
   * After enough players join, init_game() is called.
   */
  void start_lobby() {
    for (uint8_t i = 0; i < server_state->get_players_count(); ++i) {
      game_start_barrier->arrive_and_wait();

      auto player = server_state->get_player_sync(
          i);  // safe read - at this moment this slot
               // ought to be valid and written to

      AcceptedPlayer acc_msg = AcceptedPlayer(i, player);
      connector->broadcast_message(
          acc_msg);  // this is responsibility of connector to synchronize
                     // correctly
    }

    GameStarted game_started_message(*server_state);
    server_state->start_game();  // atomic
    connector->broadcast_message(
        game_started_message);  // connector's responsibility

    init_game();
  }

  /* players don't interfere with server state directly during the game, all
   * interpretations of their messages is done here, in this class
   * Here, initial players' and blocks' positions are set and
   * turn 0 is being prepared.
   */
  void init_game() {
    std::shared_ptr<Turn> init_turn = std::make_shared<Turn>(
        0);  // preparations for the game, initial positions etc

    /* Loop for randomly choosing players' initial positions
     * After we choose the posiion, we create a message out of it,
     * make changes in server_state and add this message to
     * turn0 messages
     */
    for (auto [playerId, player] : server_state->get_players()) {
      Position init_pos((uint16_t)server_state->get_rand().getNextVal() %
                            server_state->get_size_x(),
                        (uint16_t)server_state->get_rand().getNextVal() %
                            server_state->get_size_y());
      std::shared_ptr<PlayerMoved> msg =
          std::make_shared<PlayerMoved>(playerId, init_pos);
      server_state->move_player(playerId, init_pos);
      init_turn->addEvent(msg);
    }

    /* A loop doing the same as above, but with blocks' initial poisitions. */
    for (uint16_t i = 0; i < server_state->get_initial_blocks(); ++i) {
      Position init_pos;
      do {
        init_pos = Position((uint16_t)server_state->get_rand().getNextVal() %
                                server_state->get_size_x(),
                            (uint16_t)server_state->get_rand().getNextVal() %
                                server_state->get_size_y());
      } while (!server_state->place_block(init_pos));

      std::shared_ptr<BlockPlaced> msg =
          std::make_shared<BlockPlaced>(init_pos);

      init_turn->addEvent(msg);
    }
    connector->broadcast_message(*init_turn);
    server_state->add_turn_sync(init_turn);

    start_game();
  }

  void start_game() {
    for (uint16_t i = 1; i < server_state->get_game_length() + 1; ++i) {
      turn_timer.expires_after(
          boost::asio::chrono::milliseconds(server_state->get_turn_duration()));
      turn_timer.wait();
      do_one_turn(i);
    }
    end_game();
  }

  void end_game() {
    server_state
        ->get_want_to_write_to_client_messages()++;  // blocking all that got
                                                     // any messages
    std::unique_lock lk(server_state->get_client_messages_mutex());
    server_state->reset_messages_from_players_no_sync();
    connector->finish();

    server_state->reset();
    server_state->get_want_to_write_to_client_messages()--;
    server_state->wake_waiting_for_shared_client_messages();

    auto new_msg = GameEnded(server_state->get_scores());
    connector->broadcast_message(new_msg);
  }

  /* Here is the course of one round. First we block incoming messages from
   * overwriting what was collected during the last round.
   * Then it is calculated which bombs have exploded, which players have died
   * and which blocks were destroyed and after all of that, messags from
   * players that survived are being processed.
   */
  void do_one_turn(uint16_t turn_num) {
    server_state->get_want_to_write_to_client_messages()++;
    std::unique_lock lk(
        server_state
            ->get_client_messages_mutex());  // blocking saving recent messages,
                                             // since the turn has ended
    std::shared_ptr<Turn> cur_turn = std::make_shared<Turn>(turn_num);

    for (auto [id, bomb] : server_state->get_bombs()) {
      auto opt_val = server_state->check_bomb(id);
      if (opt_val) {
        auto [a, b] = *opt_val;
        std::shared_ptr<BombExploded> bomb_explosion =
            std::make_shared<BombExploded>(id, b, a);
        cur_turn->addEvent(bomb_explosion);
      }
    }

    auto dead_players = server_state->clean_up_bombs();

    for (auto [id, msg] : server_state->get_messages_from_turn_no_sync()) {
      if (!dead_players.contains(id) && msg) {
        msg->update_server_state(*server_state, id,
                                 cur_turn);  // and id, //and turn
      } else if (dead_players.contains(id)) {
        Position new_pos((uint16_t)server_state->get_rand().getNextVal() %
                             server_state->get_size_x(),
                         (uint16_t)server_state->get_rand().getNextVal() %
                             server_state->get_size_y());
        std::shared_ptr<PlayerMoved> new_msg =
            std::make_shared<PlayerMoved>(id, new_pos);
        server_state->move_player(id, new_pos);
        cur_turn->addEvent(new_msg);
      }
    }
    server_state->reset_messages_from_players_no_sync();
    connector->broadcast_message(*cur_turn);
    server_state->add_turn_sync(cur_turn);
    server_state->get_want_to_write_to_client_messages()--;
    server_state->wake_waiting_for_shared_client_messages();
  }

 public:
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
