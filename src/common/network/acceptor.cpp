#include "network/acceptor.h"

namespace light {
namespace network {
Acceptor::Acceptor(Looper &looper)
    : TcpSocket(), looper_(&looper), dispatcher_() {}

Acceptor::~Acceptor() {}

light::utils::ErrorCode Acceptor::open(const protocol::All &v) {
  auto ec = TcpSocket::open(v);
  if (ec)
    return ec;
  ec = TcpSocket::set_nonblocking();
  if (ec)
    return ec;
  dispatcher_.reset(new Dispatcher(*looper_, this->sockfd_));
  return LS_OK_ERROR();
}

void Acceptor::unset_accept_handler() {
  assert(dispatcher_);
  dispatcher_->disable_read();
}

} /* network */

} /* light */
