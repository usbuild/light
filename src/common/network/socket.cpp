#include "network/socket.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "utils/exception.h"
#include "utils/error_code.h"
#include "utils/logger.h"

using namespace light;
using namespace light::network;
using namespace std::placeholders;
//SocketOps
light::utils::ErrorCode SocketOps::open_and_bind(int &fd, const INetEndPoint& endpoint) const {
	light::utils::ErrorCode ec;
	if (endpoint.is_ipv4()) {
		ec = this->open(fd, protocol::V4());
	} else {
		ec = this->open(fd, protocol::V6());
	}

	if (!ec.ok()) return ec;

	ec = this->bind(fd, endpoint);

	if (!ec.ok()) {
		auto cec = this->close(fd);
		if (!cec.ok()) {
			return cec;
		}
	}
	return ec;
}

light::utils::ErrorCode SocketOps::bind(int fd, const INetEndPoint& endpoint) const {
	light::utils::ErrorCode ec;
	if (::bind(fd, endpoint.get_sock_addr(), endpoint.get_socklen()) < 0) {
		ec = LS_GENERIC_ERROR(errno);
	}
	return ec;
}

light::utils::ErrorCode SocketOps::connect(int fd, const INetEndPoint& endpoint) const {
	light::utils::ErrorCode ec;

	int ret = ::connect(fd, endpoint.get_sock_addr(), endpoint.get_socklen());
	if (ret != 0) {
		ec = LS_GENERIC_ERROR(errno);
	}
	return ec;
}

light::utils::ErrorCode SocketOps::listen(int fd, int backlog/*=5*/) const {
	light::utils::ErrorCode ec;
	int ret = ::listen(fd, backlog);
	if (ret == -1) {
		ec = LS_GENERIC_ERROR(errno);
	}
	return ec;
}

light::utils::ErrorCode SocketOps::close(int &fd) const {
	light::utils::ErrorCode ec;
	if (!fd) return ec;
	if (::close(fd) != 0) {
		ec = errno;
	} else {
		fd = 0;
	}
	return ec;
}

//TcpSocketOps
light::utils::ErrorCode TcpSocketOps::open(int &sockfd, const protocol::V4& v4) const noexcept {
	UNUSED(v4);
	light::utils::ErrorCode ec;
	if ((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		ec = LS_GENERIC_ERROR(errno);
		sockfd = 0;
	}
	return ec;
}

light::utils::ErrorCode TcpSocketOps::open(int &sockfd, const protocol::V6& v6) const noexcept {
	UNUSED(v6);
	light::utils::ErrorCode ec;
	if ((sockfd = ::socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
		ec = LS_GENERIC_ERROR(errno);
		sockfd = 0;
	}
	return ec;
}

//UdpSocketOps
light::utils::ErrorCode UdpSocketOps::open(int &sockfd, const protocol::V4& v4) const noexcept {
	UNUSED(v4);
	light::utils::ErrorCode ec;
	if ((sockfd = ::socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		ec = LS_GENERIC_ERROR(errno);
		sockfd = 0;
	}
	return ec;
}

light::utils::ErrorCode UdpSocketOps::open(int &sockfd, const protocol::V6& v6) const noexcept {
	UNUSED(v6);
	light::utils::ErrorCode ec;
	if ((sockfd = ::socket(AF_INET6, SOCK_DGRAM, 0)) == -1) {
		ec = LS_GENERIC_ERROR(errno);
		sockfd = 0;
	}
	return ec;
}

//for generic socket

TcpSocketOps& Socket::tcp_ops() {
	static TcpSocketOps ops;
	return ops;
}

UdpSocketOps& Socket::udp_ops() {
	static UdpSocketOps ops;
	return ops;
}
Socket::Socket(const SocketOps &ops, const INetEndPoint& endpoint): Socket(Socket::tcp_ops()){
	UNUSED(ops);
	auto ec = ops_->open_and_bind(sockfd_, endpoint);
	if (!ec.ok()) throw light::exception::SocketException(ec);
}

light::utils::ErrorCode Socket::set_nonblocking() {
	light::utils::ErrorCode ec;
	if (light::utils::set_nonblocking(this->sockfd_) != 0) {
		ec = errno;
	}
	return ec;
}

light::utils::ErrorCode Socket::set_reuseaddr(int enable) {
	light::utils::ErrorCode ec;
	int optval = enable;
	if (setsockopt(this->sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
		ec = errno;
	}
	return ec;
}
ssize_t Socket::write(light::utils::ErrorCode &ec, const void *buf, size_t len, int flags) {
	errno = 0;
	ssize_t s = ::send(this->sockfd_, buf, len, flags);
	ec = LS_GENERIC_ERROR(errno);
	return s;
}

ssize_t Socket::read(light::utils::ErrorCode &ec, void *buf, size_t len, int flags) {
	errno = 0;
	ssize_t s = ::recv(this->sockfd_, buf, len, flags);
	ec = LS_GENERIC_ERROR(errno);
	return s;
}

light::utils::ErrorCode Socket::open(const protocol::All& all) noexcept {
	if (dynamic_cast<const protocol::V4*>(&all)) {
		return ops_->open(this->sockfd_, protocol::v4());
	} else if (dynamic_cast<const protocol::V6*>(&all)){
		return ops_->open(this->sockfd_, protocol::v6());
	} else {
		return LS_GENERIC_ERR_OBJ(address_family_not_supported);
	}
}

light::utils::ErrorCode Socket::get_local_endpoint(INetEndPoint& endpoint) {
	if (!local_point_.is_ipv4() && !local_point_.is_ipv6()) {
		socklen_t len = sizeof(struct sockaddr_storage);
		struct sockaddr_storage addr;
		struct sockaddr *addr_info = reinterpret_cast<struct sockaddr*>(&addr);
		int ret = ::getsockname(get_sockfd(), addr_info, &len);
		if (ret != 0) {
			return LS_GENERIC_ERROR(errno);
		} else {
			local_point_.from_raw_struct(addr_info);
		}
	}
	endpoint = local_point_;
	return LS_OK_ERROR();
}

//---------------for TcpSocket
light::utils::ErrorCode TcpSocket::accept(TcpSocket& client_socket) {
	return accept(client_socket.sockfd_);
}
light::utils::ErrorCode TcpSocket::accept(int& fd) {
	light::utils::ErrorCode ec;
	socklen_t socklen;
	int ret = ::accept(this->sockfd_, nullptr, nullptr);
	if (ret <= 0) {
		ec = LS_GENERIC_ERROR(errno);
		return ec;
	}
	fd = ret;
	return ec;
}
light::utils::ErrorCode TcpSocket::set_keepalive() {
	light::utils::ErrorCode ec;
	int optval;
	if (setsockopt(this->sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) != 0) {
		ec = errno;
	}
	return ec;
}
