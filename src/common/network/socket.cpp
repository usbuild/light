#include "network/socket.h"
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <fcntl.h>
#include <memory>
#include <mutex>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "utils/exception.h"
#include "utils/error_code.h"
#include "utils/logger.h"

using namespace light;
using namespace light::network;
using namespace std::placeholders;
// SocketOps
std::error_code
SocketOps::open_and_bind(int &fd, const INetEndPoint &endpoint) const {
  std::error_code ec;
  if (endpoint.is_ipv4()) {
    ec = this->open(fd, protocol::V4());
  } else {
    ec = this->open(fd, protocol::V6());
  }

  if (ec)
    return ec;

  ec = this->bind(fd, endpoint);

  if (ec) {
    auto cec = this->close(fd);
    if (cec) {
      return cec;
    }
  }
  return ec;
}

std::error_code SocketOps::bind(int fd,
                                        const INetEndPoint &endpoint) const {
  std::error_code ec;
  if (::bind(fd, endpoint.get_sock_addr(), endpoint.get_socklen()) < 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  }
  return ec;
}

std::error_code SocketOps::connect(int fd,
                                           const INetEndPoint &endpoint) const {
  std::error_code ec;

  int ret = ::connect(fd, endpoint.get_sock_addr(), endpoint.get_socklen());
  if (ret != 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  }
  return ec;
}

std::error_code SocketOps::listen(int fd, int backlog /*=5*/) const {
  std::error_code ec;
  int ret = ::listen(fd, backlog);
  if (ret == -1) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  }
  return ec;
}

std::error_code SocketOps::close(int &fd) const {
  std::error_code ec;
  if (!fd)
    return ec;
  if (socketclose(fd) != 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  } else {
    fd = 0;
  }
  return ec;
}

// TcpSocketOps
std::error_code TcpSocketOps::open(int &sockfd,
                                           const protocol::V4 &v4) const
    noexcept {
  UNUSED(v4);
  std::error_code ec;
  if ((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
    sockfd = 0;
  }
  return ec;
}

std::error_code TcpSocketOps::open(int &sockfd,
                                           const protocol::V6 &v6) const
    noexcept {
  UNUSED(v6);
  std::error_code ec;
  if ((sockfd = ::socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
    sockfd = 0;
  }
  return ec;
}

// UdpSocketOps
std::error_code UdpSocketOps::open(int &sockfd,
                                           const protocol::V4 &v4) const
    noexcept {
  UNUSED(v4);
  std::error_code ec;
  if ((sockfd = ::socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
    sockfd = 0;
  }
  return ec;
}

std::error_code UdpSocketOps::open(int &sockfd,
                                           const protocol::V6 &v6) const
    noexcept {
  UNUSED(v6);
  std::error_code ec;
  if ((sockfd = ::socket(AF_INET6, SOCK_DGRAM, 0)) == -1) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
    sockfd = 0;
  }
  return ec;
}

// for generic socket

TcpSocketOps &Socket::tcp_ops() {
  static TcpSocketOps ops;
  return ops;
}

UdpSocketOps &Socket::udp_ops() {
  static UdpSocketOps ops;
  return ops;
}
Socket::Socket(const SocketOps &ops, const INetEndPoint &endpoint)
    : Socket(Socket::tcp_ops()) {
  UNUSED(ops);
  auto ec = ops_->open_and_bind(sockfd_, endpoint);
  if (ec)
    throw light::exception::SocketException(ec);
}

std::error_code Socket::set_nonblocking() {
  std::error_code ec;
  if (light::utils::set_nonblocking(this->sockfd_) != 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  }
  return ec;
}

std::error_code Socket::set_reuseaddr(int enable) {
  std::error_code ec;
#ifdef WIN32
  const char optval = static_cast<char>(enable);
#else
  int optval = enable;
#endif
  if (setsockopt(this->sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) != 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  }
  return ec;
}
ssize_t Socket::write(std::error_code &ec, const void *buf, size_t len,
                      int flags) {
  ssize_t s = ::send(this->sockfd_, static_cast<const char *>(buf), len, flags);
  ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  return s;
}

ssize_t Socket::read(std::error_code &ec, void *buf, size_t len,
                     int flags) {
  ssize_t s = ::recv(this->sockfd_, static_cast<char *>(buf), len, flags);
  ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  return s;
}

std::error_code Socket::open(const protocol::All &all) noexcept {
  if (dynamic_cast<const protocol::V4 *>(&all)) {
    return ops_->open(this->sockfd_, protocol::v4());
  } else if (dynamic_cast<const protocol::V6 *>(&all)) {
    return ops_->open(this->sockfd_, protocol::v6());
  } else {
    return LS_GENERIC_ERR_OBJ(address_family_not_supported);
  }
}

std::error_code Socket::get_local_endpoint(INetEndPoint &endpoint) {
  if (!local_point_.is_ipv4() && !local_point_.is_ipv6()) {
    socklen_t len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage addr;
    struct sockaddr *addr_info = reinterpret_cast<struct sockaddr *>(&addr);
    int ret = ::getsockname(get_sockfd(), addr_info, &len);
    if (ret != 0) {
      return LS_GENERIC_ERROR(SOCK_ERRNO());
    } else {
      local_point_.from_raw_struct(addr_info);
    }
  }
  endpoint = local_point_;
  return LS_OK_ERROR();
}

//---------------for TcpSocket
std::error_code TcpSocket::accept(TcpSocket &client_socket) {
  return accept(client_socket.sockfd_);
}
std::error_code TcpSocket::accept(int &fd) {
  std::error_code ec;
  int ret = ::accept(this->sockfd_, nullptr, nullptr);
  if (ret <= 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
    return ec;
  }
  fd = ret;
  return ec;
}
std::error_code TcpSocket::set_keepalive() {
  std::error_code ec;
#ifdef WIN32
  char optval = 1;
#else
  int optval = 1;
#endif
  if (setsockopt(this->sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
                 sizeof(optval)) != 0) {
    ec = LS_GENERIC_ERROR(SOCK_ERRNO());
  }
  return ec;
}
