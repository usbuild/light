#include "network/endpoint.h"
#include "utils/helpers.h"
using namespace light;
using namespace light::network;

namespace light {
namespace network {

INetEndPointIpV4::INetEndPointIpV4(const std::string &ip, int port) {
  auto ec = this->parse_from_ip_port(ip, port);
  if (!ec.ok()) {
    throw light::exception::SocketException(ec);
  }
}

INetEndPointIpV4::operator INetEndPoint() const { return INetEndPoint(*this); }

light::utils::ErrorCode
INetEndPointIpV4::parse_from_ip_port(const std::string &ip, int port) {
  light::utils::ErrorCode ec;
  clear_sockaddr();
  addr4_.sin_family = AF_INET;

  if (ip.empty()) {
    addr4_.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    if (::inet_pton(AF_INET, ip.c_str(), &(addr4_.sin_addr)) <= 0) {
      ec = LS_GENERIC_ERROR(errno);
    }
  }
  addr4_.sin_port = htons(port);
  return ec;
}

void INetEndPointIpV4::from_raw_struct(struct sockaddr_in *addr) {
  memcpy(&addr4_, addr, sizeof(struct sockaddr_in));
}

INetEndPointIpV6::operator INetEndPoint() const { return INetEndPoint(*this); }

sa_family_t INetEndPoint::get_ip_version(const std::string &ip,
                                         light::utils::ErrorCode &ec) {
  struct addrinfo hint, *res = NULL;

  memset(&hint, '\0', sizeof hint);

  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  if (getaddrinfo(ip.c_str(), NULL, &hint, &res) != 0) {
    ec = LS_GENERIC_ERROR(errno);
    return AF_UNSPEC;
  }

  auto v = res->ai_family;
  freeaddrinfo(res);
  return v;
}

INetEndPointIpV6::INetEndPointIpV6(const std::string &ip, int port) {
  auto ec = this->parse_from_ip_port(ip, port);
  if (!ec.ok()) {
    throw light::exception::SocketException(ec);
  }
}

std::string INetEndPointIpV6::to_string() const {
  char buf[INET6_ADDRSTRLEN] = {0};
  auto addr = addr6_.sin6_addr;
  ::inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN);
  return std::string(buf);
}

light::utils::ErrorCode
INetEndPointIpV6::parse_from_ip_port(const std::string &ip, int port) {
  light::utils::ErrorCode ec;
  clear_sockaddr();
  addr6_.sin6_family = AF_INET6;

  if (ip.empty()) {
    addr6_.sin6_addr = in6addr_any;
  } else {
    if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0) {
      ec = LS_GENERIC_ERROR(errno);
    }
  }
  addr6_.sin6_port = htons(port);
  return ec;
}

void INetEndPointIpV6::from_raw_struct(struct sockaddr_in6 *addr) {
  memcpy(&addr6_, addr, sizeof(struct sockaddr_in6));
}

INetEndPoint::INetEndPoint(const protocol::V4 &v4, int port) {
  UNUSED(v4);
  auto ec = addr_.ipv4.parse_from_ip_port("", port);
  if (!ec.ok())
    throw light::exception::SocketException(ec);
  family_ = AF_INET;
}

INetEndPoint::INetEndPoint(const protocol::V6 &v6, int port) {
  UNUSED(v6);
  auto ec = addr_.ipv6.parse_from_ip_port("", port);
  if (!ec.ok())
    throw light::exception::SocketException(ec);
  family_ = AF_INET6;
}

INetEndPoint::INetEndPoint(const std::string &ip, int port) {
  light::utils::ErrorCode ec;
  *this = INetEndPoint::parse_from_ip_port(ec, ip, port);
  if (!ec.ok())
    throw light::exception::SocketException(ec);
}

INetEndPoint::INetEndPoint(const INetEndPoint &src) { *this = src; }

INetEndPoint &INetEndPoint::operator=(const INetEndPointIpV4 &src) {
  addr_.ipv4 = src;
  family_ = AF_INET;
  return *this;
}

INetEndPoint &INetEndPoint::operator=(const INetEndPointIpV6 &src) {
  addr_.ipv6 = src;
  family_ = AF_INET6;
  return *this;
}

INetEndPoint &INetEndPoint::operator=(const INetEndPoint &src) {
  family_ = src.family_;
  addr_ = src.addr_;
  return *this;
}

const struct sockaddr *INetEndPoint::get_sock_addr() const {
  if (is_ipv4()) {
    return addr_.ipv4.get_sock_addr();
  } else if (is_ipv6()) {
    return addr_.ipv6.get_sock_addr();
  } else {
    return nullptr;
  }
}

std::string INetEndPoint::to_string() const {
  if (is_ipv4()) {
    return addr_.ipv4.to_string();
  } else if (is_ipv6()) {
    return addr_.ipv6.to_string();
  } else {
    return "unsupported";
  }
}
socklen_t INetEndPoint::get_socklen() const {
  if (is_ipv4()) {
    return addr_.ipv4.get_socklen();
  } else if (is_ipv6()) {
    return addr_.ipv6.get_socklen();
  } else {
    return 0;
  }
}

protocol::All &INetEndPoint::get_protocol() const {
  if (family_ == AF_INET) {
    return protocol::v4();
  } else if (family_ == AF_INET6) {
    return protocol::v6();
  } else {
    return protocol::unknown();
  }
}

INetEndPoint INetEndPoint::parse_from_ip_port(light::utils::ErrorCode &ec,
                                              const std::string &ip, int port) {
  INetEndPoint ret;
  auto ver = INetEndPoint::get_ip_version(ip, ec);
  if (!ec.ok()) {
    return ret;
  }
  if (ver == AF_INET) {
    auto endpoint = INetEndPointIpV4();
    ec = endpoint.parse_from_ip_port(ip, port);
    if (ec.ok()) {
      ret = endpoint;
    }

  } else if (ver == AF_INET6) {
    auto endpoint = INetEndPointIpV6();
    ec = endpoint.parse_from_ip_port(ip, port);
    if (ec.ok()) {
      ret = endpoint;
    };
  } else {
    ec = light::utils::generic_errors::basic::address_family_not_supported;
  }
  return ret;
}

void INetEndPoint::from_raw_struct(struct sockaddr *addr) {
  family_ = addr->sa_family;
  if (family_ == AF_INET) {
    addr_.ipv4.from_raw_struct(reinterpret_cast<struct sockaddr_in *>(addr));
  } else {
    addr_.ipv6.from_raw_struct(reinterpret_cast<struct sockaddr_in6 *>(addr));
  }
}

unsigned int INetEndPoint::get_addr_int() const {
  if (family_ == AF_INET) {
    return addr_.ipv4.get_addr_int();
  } else if (family_ == AF_INET6) {
    return addr_.ipv6.get_addr_int();
  } else {
    return 0;
  }
}

unsigned int INetEndPoint::get_port() const {
  if (family_ == AF_INET) {
    return addr_.ipv4.get_port();
  } else if (family_ == AF_INET6) {
    return addr_.ipv6.get_port();
  } else {
    return 0;
  }
}

} /* network */
} /* light */
