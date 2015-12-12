#pragma once
#include "network/dispatcher.h"
#include "network/looper.h"
#include "network/socket.h"
#include "network/timer.h"
namespace light {
namespace network {

class Looper;

class TcpClient : public TcpSocket,
                  public std::enable_shared_from_this<TcpClient> {
public:
  TcpClient(Looper &looper);
  ~TcpClient();

  std::error_code open(const protocol::All &v);

  template <typename ConnectHandler>
  void async_connect(const INetEndPoint &point,
                     const ConnectHandler &on_connect,
                     uint64_t timeout = 76 * 1000000LL);

private:
  Looper *looper_;
  std::unique_ptr<Dispatcher> dispatcher_;
};

template <typename ConnectHandler>
void TcpClient::async_connect(const INetEndPoint &point,
                              const ConnectHandler &on_connect,
                              uint64_t timeout) {
  auto ec = TcpSocket::connect(point);
  if (ec.value() == CERR(EINPROGRESS) || ec.value() == CERR(EWOULDBLOCK) ) {
    dispatcher_.reset(new Dispatcher(*looper_, this->sockfd_));

    auto timer_id = looper_->add_timer(ec, timeout, 0, [this, on_connect] {
      dispatcher_->detach();
      on_connect(LS_GENERIC_ERR_OBJ(timed_out));
    });
    auto handle = [on_connect, this, timer_id] {
      auto sec = this->get_last_error();
      dispatcher_->detach();
      // rescue this dispatcher from loop and prepare for connection
      looper_->cancel_timer(sec, timer_id);
      on_connect(sec);
    };

    dispatcher_->set_write_callback(handle);

    dispatcher_->enable_write();

  } else {
    on_connect(ec);
  }
}
} /* network */
} /* light */
