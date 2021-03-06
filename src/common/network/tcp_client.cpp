#include "network/tcp_client.h"
namespace light {
namespace network {

TcpClient::TcpClient(Looper &looper) : TcpSocket(), looper_(&looper) {}

TcpClient::~TcpClient() {
	DLOG(INFO) << __FUNCTION__ << " " << this;
  if (dispatcher_) {
    dispatcher_->detach();
  }
}

std::error_code TcpClient::open(const protocol::All &v) {
  auto ec = TcpSocket::open(v);
  if (ec)
    return ec;
  ec = TcpSocket::set_nonblocking();
  if (ec)
    return ec;
  return LS_OK_ERROR();
}
} /* network */
} /* light */
