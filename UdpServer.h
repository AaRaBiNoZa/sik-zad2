//
// Created by Adam Al-Hosam on 04/05/2022.
//

#ifndef SIK_ZAD3_UDPSERVER_H
#define SIK_ZAD3_UDPSERVER_H

#include "ByteStream.h"
#include "Messages.h"
#include "boost/array.hpp"
#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "common.h"

using boost::asio::ip::resolver_base;
using boost::asio::ip::udp;

class UdpServer {
 private:
  std::shared_ptr<udp::socket> socket;
  udp::endpoint display_endpoint;  // not needed i think
  ByteStream recv_bstream;

  void init(const boost::system::error_code& error) {
    std::cerr << "GOT HERE";
    if (!error) {
      start_receive();
    }
  }
  void start_receive() {
    //    socket->async_receive(boost::asio::buffer(recv_bstream.getData()),
    //                          boost::bind(&UdpServer::handle_receive, this,
    //                                      boost::asio::placeholders::error));
    socket->async_wait(boost::asio::ip::udp::socket::wait_read,
                       boost::bind(&UdpServer::handle_receive, this,
                                   boost::asio::placeholders::error));
  }

  void handle_receive(const boost::system::error_code& error) {
    recv_bstream.reset();
    std::shared_ptr<Message> k = InputMessage::unserialize(recv_bstream);
    recv_bstream.endRec();
    k->say_hello();
    if (!error) {
      boost::shared_ptr<std::string> message(new std::string("LOL"));

      socket->async_send(
          boost::asio::buffer(*message),
          boost::bind(&UdpServer::handle_send, this, message,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));

      start_receive();
    }
  }

  void handle_send(const boost::shared_ptr<std::string>& /*message*/,
                   const boost::system::error_code&, std::size_t) {
  }

  // tu try catch
 public:
  UdpServer(boost::asio::io_context& io_context, uint16_t port,
            std::string display_addr)
      : socket(std::make_shared<udp::socket>(io_context,udp::endpoint(udp::v4(),port))),
        recv_bstream(std::make_unique<UdpStreamBuffer>(socket)) {
    auto [display_host, display_port] = extract_host_and_port(display_addr);

    udp::resolver udp_resolver(io_context);
    display_endpoint = *udp_resolver.resolve(display_host, display_port,
                                             resolver_base::numeric_service);

    try {
      socket->async_connect(display_endpoint,
                            boost::bind(&UdpServer::init, this,
                                        boost::asio::placeholders::error));
    } catch (std::exception& e) {
      std::cerr << "Error: " << e.what() << "\n";
      exit(1);
    } catch (...) {
      std::cerr << "Unknown error!"
                << "\n";
      exit(1);
    }

    start_receive();
  }
};
#endif  // SIK_ZAD3_UDPSERVER_H
