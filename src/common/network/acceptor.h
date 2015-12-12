#pragma once
#include "network/socket.h"
#include "network/dispatcher.h"
namespace light {
namespace network {

class Looper;

class Acceptor : public TcpSocket,
                 public std::enable_shared_from_this<Acceptor> {
public:
  Acceptor(Looper &looper);
  ~Acceptor();

  std::error_code open(const protocol::All &v);

  /**
   * @brief 用于接收一个客户端请求
   *
   * @tparam AcceptHandler
   * @param accept_handler 连接建立后的回调函数
   */
  template <typename AcceptHandler>
  void async_accept_one(AcceptHandler accept_handler);

  /**
   * @brief 指定一个接收新建立连接的回调函数，除非unset，否则一直会调用。
   * 当连接超过最大文件树时会导致cpu占用率100%，查看<a
   * href="http://search.cpan.org/~mlehmann/EV-4.21/libev/ev.pod#The_special_problem_of_accept()ing_when_you_can't">这里</a>
   * @tparam AcceptHandler
   * @param accept_handler
   */
  template <typename AcceptHandler>
  void set_accept_handler(AcceptHandler accept_handler);

  void unset_accept_handler();

  template <typename CloseHandler>
  void set_on_close(CloseHandler close_handler);

  template <typename ErrorHandler>
  void set_on_error(ErrorHandler error_handler);

private:
  Looper *looper_;
  std::unique_ptr<Dispatcher> dispatcher_;
};

template <typename AcceptHandler>
void Acceptor::async_accept_one(AcceptHandler accept_handler) {
  assert(dispatcher_);

  dispatcher_->enable_read();
  dispatcher_->set_read_callback([this, accept_handler] {
    int client_sock;
    auto ec = this->accept(client_sock);
    dispatcher_->disable_read();
    accept_handler(ec, client_sock);
  });
}

template <typename AcceptHandler>
void Acceptor::set_accept_handler(AcceptHandler accept_handler) {
  assert(dispatcher_);

  dispatcher_->enable_read();
  dispatcher_->set_read_callback([this, accept_handler] {
    int fd;
    auto ec = this->accept(fd);
    accept_handler(ec, fd);
  });
}

template <typename CloseHandler>
void Acceptor::set_on_close(CloseHandler close_handler) {
  assert(dispatcher_);
  dispatcher_->set_close_callback(close_handler);
}

template <typename ErrorHandler>
void Acceptor::set_on_error(ErrorHandler error_handler) {
  assert(dispatcher_);
  dispatcher_->set_error_callback(error_handler);
}

} /* network */

} /* light */
