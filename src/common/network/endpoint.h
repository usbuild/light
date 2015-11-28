#pragma once
#include "config.h"

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_WIN_SOCK2_H
#include <WinSock2.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif
#ifdef HAVE_WS2_TCPIP_H
#include <WS2tcpip.h>
#endif

#include <memory>
#include <stdint.h>
#include <string>
#include "utils/error_code.h"
#include "utils/exception.h"
#include "utils/noncopyable.h"
#include "utils/platform.h"

namespace light {
namespace network {

namespace protocol {

class All : public light::utils::NonCopyable {
protected:
  All() {}

public:
  virtual ~All() {}
};

class V4 : public All {};
class V6 : public All {};
class Unknown : public All {};

static V4 &v4() {
  static V4 v;
  return v;
}

static V6 &v6() {
  static V6 v;
  return v;
}

static Unknown &unknown() {
  static Unknown v;
  return v;
}

} /* protocol */

class INetEndPoint;

class INetEndPointIpV4 {
public:
  INetEndPointIpV4() { clear_sockaddr(); }

  INetEndPointIpV4(const std::string &ip, int port);

  operator INetEndPoint() const;

  const struct sockaddr *get_sock_addr() const {
    return reinterpret_cast<const sockaddr *>(&addr4_);
  }

  light::utils::ErrorCode parse_from_ip_port(const std::string &ip, int port);

  std::string to_string() const {
    return std::string(::inet_ntoa(addr4_.sin_addr));
  }

  socklen_t get_socklen() const { return sizeof(addr4_); }

  unsigned int get_addr_int() const { return addr4_.sin_addr.s_addr; }
  void set_addr_int(unsigned int addr) { addr4_.sin_addr.s_addr = addr; }
  unsigned int get_port() const { return ntohs(addr4_.sin_port); }
  void set_port(unsigned int port) { addr4_.sin_port = htons(port); }
  void from_raw_struct(struct sockaddr_in *addr);

private:
  void clear_sockaddr() { memset(&addr4_, 0, sizeof(addr4_)); }
  struct sockaddr_in addr4_;
};

class INetEndPointIpV6 {
public:
  INetEndPointIpV6() { clear_sockaddr(); }
  operator INetEndPoint() const;

  INetEndPointIpV6(const std::string &ip, int port);

  const struct sockaddr *get_sock_addr() const {
    return reinterpret_cast<const sockaddr *>(&addr6_);
  }

  std::string to_string() const;

  light::utils::ErrorCode parse_from_ip_port(const std::string &ip, int port);

  socklen_t get_socklen() const { return sizeof(addr6_); }

  unsigned int get_addr_int() const { return 0; }
  unsigned int get_port() const { return ntohs(addr6_.sin6_port); }
  void from_raw_struct(struct sockaddr_in6 *addr);

private:
  void clear_sockaddr() { memset(&addr6_, 0, sizeof(addr6_)); }

  struct sockaddr_in6 addr6_;
};

class INetEndPoint {
public:
  INetEndPoint() : family_(AF_UNSPEC) {}
  INetEndPoint(const INetEndPoint &);

  explicit INetEndPoint(const INetEndPointIpV4 &src) { *this = src; }

  explicit INetEndPoint(const INetEndPointIpV6 &src) { *this = src; }

  INetEndPoint(const protocol::V4 &v4, int port);

  INetEndPoint(const protocol::V6 &v6, int port);

  INetEndPoint(const std::string &ip, int port);

  INetEndPoint &operator=(const INetEndPointIpV4 &src);

  INetEndPoint &operator=(const INetEndPointIpV6 &src);

  INetEndPoint &operator=(const INetEndPoint &src);

  const struct sockaddr *get_sock_addr() const;

  INetEndPointIpV4 get_ipv4() { return addr_.ipv4; }

  INetEndPointIpV6 get_ipv6() { return addr_.ipv6; }

  inline bool is_ipv4() const { return family_ == AF_INET; }

  inline bool is_ipv6() const { return family_ == AF_INET6; }

  std::string to_string() const;

  socklen_t get_socklen() const;

  protocol::All &get_protocol() const;

  unsigned int get_addr_int() const;
  unsigned int get_port() const;

  static sa_family_t get_ip_version(const std::string &ip,
                                    light::utils::ErrorCode &ec);

  static INetEndPoint parse_from_ip_port(light::utils::ErrorCode &ec,
                                         const std::string &ip, int port);

  void from_raw_struct(struct sockaddr *addr);

private:
  typedef union INetEndPointIpV46 {
    INetEndPointIpV4 ipv4;
    INetEndPointIpV6 ipv6;
    INetEndPointIpV46() { memset(this, 0, sizeof(INetEndPointIpV46)); }
  } INetEndPointIpV46;

  INetEndPointIpV46 addr_;
  sa_family_t family_;
};

} /* network */
}
