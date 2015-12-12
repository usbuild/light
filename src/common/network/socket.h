#pragma once

#include <memory>

#include <string>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_WIN_SOCK2_H
#include <WinSock2.h>
#endif
#ifdef HAVE_WS2_TCPIP_H
#include <ws2tcpip.h>
#endif
#include "utils/error_code.h"
#include "utils/logger.h"
#include "utils/helpers.h"
#include "utils/noncopyable.h"
#include "network/endpoint.h"

namespace light {
namespace network {

class SocketOps {
public:
  virtual std::error_code open(int &fd, const protocol::V4 &v4) const
      noexcept = 0;
  virtual std::error_code open(int &fd, const protocol::V6 &v6) const
      noexcept = 0;

  std::error_code open_and_bind(int &fd,
                                        const INetEndPoint &endpoint) const;

  std::error_code bind(int fd, const INetEndPoint &endpoint) const;

  std::error_code connect(int fd, const INetEndPoint &endpoint) const;

  std::error_code listen(int fd, int backlog /*=5*/) const;

  virtual std::error_code close(int &fd) const;
};

class TcpSocketOps : public SocketOps { /*{{{*/
public:
  std::error_code open(int &sockfd, const protocol::V4 &v4) const
      noexcept;

  std::error_code open(int &sockfd, const protocol::V6 &v6) const
      noexcept;

}; /*}}}*/

class UdpSocketOps : public SocketOps {
public:
  std::error_code open(int &sockfd, const protocol::V4 &v4) const
      noexcept;

  std::error_code open(int &sockfd, const protocol::V6 &v6) const
      noexcept;
};

class Socket : public light::utils::NonCopyable {
public:
  static TcpSocketOps &tcp_ops();

  static UdpSocketOps &udp_ops();

  virtual ~Socket() {}
  Socket(const SocketOps &ops) : sockfd_(0), ops_(&ops) {}
  Socket(const SocketOps &ops, int fd) : sockfd_(fd), ops_(&ops) {}

  explicit Socket(const SocketOps &ops, const INetEndPoint &endpoint);

  virtual std::error_code set_nonblocking();

  virtual std::error_code set_reuseaddr(int enable);

  virtual std::error_code get_last_error() {
    return light::utils::check_socket_error(sockfd_);
  }

  virtual std::error_code bind(const INetEndPoint &endpoint) {
    return ops_->bind(sockfd_, endpoint);
  }
  virtual std::error_code listen(int backlog = 5) {
    return ops_->listen(sockfd_, backlog);
  }
  virtual std::error_code connect(const INetEndPoint &endpoint) {
    return ops_->connect(sockfd_, endpoint);
  }

  std::error_code open(const protocol::V4 &v4) noexcept {
    return ops_->open(this->sockfd_, v4);
  }
  std::error_code open(const protocol::V6 &v6) noexcept {
    return ops_->open(this->sockfd_, v6);
  }

  std::error_code open(const protocol::All &all) noexcept;

  std::error_code open(const INetEndPoint &endpoint) noexcept {
    return ops_->open_and_bind(this->sockfd_, endpoint);
  }

  virtual std::error_code close() { return ops_->close(this->sockfd_); }

  virtual ssize_t write(std::error_code &ec, const void *buf,
                        size_t len, int flags = 0);

  virtual ssize_t read(std::error_code &ec, void *buf, size_t len,
                       int flags = 0);

  inline int get_sockfd() const { return sockfd_; }

  std::error_code get_local_endpoint(INetEndPoint &endpoint);

protected:
  int sockfd_;
  INetEndPoint local_point_;
  const SocketOps *ops_;
};

class TcpSocket : public Socket {
public:
  TcpSocket() : Socket(Socket::tcp_ops()) {}
  TcpSocket(int fd) : Socket(Socket::tcp_ops(), fd) {}
  explicit TcpSocket(const INetEndPoint &endpoint)
      : Socket(Socket::tcp_ops(), endpoint) {}

  std::error_code accept(TcpSocket &client_socket);
  std::error_code accept(int &fd);

  std::error_code set_keepalive();
};

class UdpSocket : public Socket {
public:
  UdpSocket() : Socket(Socket::udp_ops()) {}
  UdpSocket(int fd) : Socket(Socket::udp_ops(), fd) {}
  explicit UdpSocket(const INetEndPoint &endpoint)
      : Socket(Socket::udp_ops(), endpoint) {}
};
} /* network */
}
