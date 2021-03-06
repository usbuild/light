#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include "network/acceptor.h"
#include "network/looper.h"
#include "network/socket.h"
#include "network/tcp_client.h"
#include "network/tcp_connection.h"
#include "utils/error_code.h"
#include "utils/logger.h"
#include "enet/enet.h"

using namespace light::utils;
using namespace light::network;
using namespace std::placeholders;

TEST(Socket, demo) { /*{{{*/
  std::error_code ec;
  light::network::TcpSocket socket(
      INetEndPoint(light::network::protocol::v4(), 0));
  socket.set_reuseaddr(1);
  ec = socket.connect(INetEndPoint("93.184.216.34", 80));
  DLOG(INFO) << ec.message();
  std::string hello("GET / HTTP/1.1\r\n"
                    "User-Agent: curl/7.26.0\r\n"
                    "Host: example.org\r\n"
                    "Accept: */*\r\n\r\n");
  ssize_t size = socket.write(ec, hello.data(), hello.size());
  char buf[1024] = {0};
  socket.read(ec, buf, 1023);
	DLOG(INFO) << buf;
} /*}}}*/

class EchoConnection {/*{{{*/
public:
  EchoConnection(Looper &looper, int fd)
      : looper_(nullptr), conn_(new TcpConnection(looper, fd)), close_(false) {}

  void handle(const std::error_code &ec, size_t bytes_transferred) {
    UNUSED(bytes_transferred);
    if (!ec) {
      ::memcpy(wbuf_, rbuf_, 1024);
      conn_->async_write(wbuf_, strlen(static_cast<char *>(wbuf_)) + 1,
                         []() {});
      ::memset(rbuf_, 0, 1024);
      conn_->async_read_some(rbuf_, 1024,
                             std::bind(&EchoConnection::handle, this, _1, _2));
    } else {
      DLOG(INFO) << "ERROR: " << ec.message();
    }
  }

  void run() {
    conn_->set_error_callback([this] {
      DLOG(INFO) << conn_->get_last_error().message();
      conn_->close();
    });

    conn_->set_close_callback([this] { conn_->close(); });

    conn_->async_read_some(rbuf_, 1024,
                           std::bind(&EchoConnection::handle, this, _1, _2));
  }

private:
  Looper *looper_;
  std::unique_ptr<TcpConnection> conn_;
  bool close_;
  char wbuf_[1025];
  char rbuf_[1025];
}; /*}}}*/

TEST(Acceptor, accept) { /*{{{*/
  Looper looper;
  Acceptor acceptor(looper);
  INetEndPoint endpoint(protocol::v4(), 8888);
  acceptor.open(endpoint.get_protocol());
  acceptor.set_reuseaddr(1);
  acceptor.bind(endpoint);
  acceptor.listen();

  acceptor.set_accept_handler([&looper](std::error_code &ec, int fd) {
    UNUSED(ec);
    DLOG(INFO) << "get client: " << fd;
    (new EchoConnection(looper, fd))->run();
  });

  //	looper.loop();
} /*}}}*/

TEST(TcpClient, accept) { /*{{{*/
  Looper looper;
  TcpClient tcp_client(looper);
  tcp_client.open(protocol::v4());

  char *buf = new char[1025];
  char *wbuf = new char[1025];
  
  TcpConnection *conn = nullptr;
  tcp_client.async_connect(
      INetEndPoint("93.184.216.34", 80),
      [&tcp_client, &looper, buf, wbuf,
       &conn](const std::error_code &ec) {
        DLOG(INFO) << ec.message() << "  message  " << ec.value();
        if (!ec) {
          conn = new TcpConnection(looper, tcp_client.get_sockfd());
          std::string hello("GET / HTTP/1.1\r\n"
                            "User-Agent: curl/7.26.0\r\n"
                            "Host: example.org\r\n"
                            "Accept: */*\r\n\r\n");
          ::memcpy(wbuf, hello.data(), hello.size());
          conn->async_write(wbuf, hello.size(), [] { EXPECT_TRUE(true); });
          ::memset(buf, 0, 1025);
          conn->async_read_some(
              buf, 1024, [buf, &looper, conn, wbuf](const std::error_code &ec,
                                        int bytes_transferred) {
                DLOG(INFO) << ec.message();
                UNUSED(ec);
                UNUSED(bytes_transferred);
                DLOG(INFO) << buf;
                EXPECT_TRUE(true);
                looper.stop();
				        delete[]buf;
				        delete[]wbuf;
				        delete conn;
              });
        } else {
          EXPECT_TRUE(false);
          DLOG(INFO) << ec.message();
        }
      },
      10 * 1000000LL);
  looper.loop();
} /*}}}*/

TEST(Looper, demo) { /*{{{*/
  Looper looper;
  std::error_code ec;
  int i = 0;
  uintptr_t timer_id = 0;

  timer_id =
      looper.add_timer(ec, 1000000LL, 1000000LL, [&i, &timer_id, &ec, &looper] {
        i += 1;
        if (i == 2) {
          DLOG(INFO) << "trigger 2";
        } else if (i == 3) {
          looper.cancel_timer(ec, timer_id);
          looper.stop();
        } else {
          DLOG(INFO) << "trigger";
        }
      });

  looper.add_timer(ec, 1500000LL, 0, [&looper] { DLOG(INFO) << "one shot"; });
  looper.loop();
} /*}}}*/
