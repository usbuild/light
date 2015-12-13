#include "network/tcp_connection.h"
namespace light {
namespace network {

TcpConnection::TcpConnection(Looper &looper)
    : TcpSocket(), Connection(looper), write_buffer_(), bytes_has_read_(0) {}

TcpConnection::TcpConnection(Looper &looper, int fd)
    : TcpSocket(fd), Connection(looper), write_buffer_(), bytes_has_read_(0) {
  dispatcher_.reset(new Dispatcher(looper, fd));
  auto ec = this->set_nonblocking();
  if (ec)
    throw light::exception::SocketException(ec);
  dispatcher_->set_write_callback(
      std::bind(&TcpConnection::buffer_write_callback, this));
}

TcpConnection::~TcpConnection() {
  if (dispatcher_)
    dispatcher_->detach();
}

void TcpConnection::buffer_write_callback() {
  assert(write_buffer_.size());

  auto &vec = write_buffer_.get_iovec();
  ssize_t written = ::writev(get_sockfd(), &vec[0], vec.size());
  if (written < 0) {
    if (SOCK_ERRNO() == EAGAIN || SOCK_ERRNO() == CERR(EWOULDBLOCK)) {

    } else {
      LOG(WARNING) << "write failed: " << LS_GENERIC_ERROR(SOCK_ERRNO()).message();
    }
  } else {
    write_buffer_.shift(written);
    if (!write_buffer_.size()) {
      dispatcher_->disable_write();
    }
  }
}

std::error_code TcpConnection::close() {
  dispatcher_->detach();
  return TcpSocket::close();
}

void TcpConnection::async_write(void *buf, size_t len,
                                const write_callback_t &func) {
  auto old_size = write_buffer_.size();
  write_buffer_.append(buf, len, func);
  if (!old_size) {
    dispatcher_->enable_write();
  }
}

std::error_code
TcpConnection::get_peer_endpoint(light::network::INetEndPoint &endpoint) {
  if (!peer_point_.is_ipv4() && !peer_point_.is_ipv6()) {
    socklen_t len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage addr;
    struct sockaddr *addr_info = reinterpret_cast<struct sockaddr *>(&addr);
    int ret = ::getpeername(get_sockfd(), addr_info, &len);
    if (ret != 0) {
      return LS_GENERIC_ERROR(SOCK_ERRNO());
    } else {
      peer_point_.from_raw_struct(addr_info);
    }
  }
  endpoint = peer_point_;
  return LS_OK_ERROR();
}

} /* network */

} /* light */
