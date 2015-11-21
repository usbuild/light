#pragma once

#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include "utils/error_code.h"
#include "utils/logger.h"
#include "utils/helpers.h"
#include "utils/noncopyable.h"
#include "network/endpoint.h"

namespace light {
	namespace network {

		class SocketOps {
		public:
			virtual light::utils::ErrorCode open(int &fd, const protocol::V4& v4) const noexcept = 0;
			virtual light::utils::ErrorCode open(int &fd, const protocol::V6& v6) const noexcept = 0;

			light::utils::ErrorCode open_and_bind(int &fd, const INetEndPoint& endpoint) const;

			light::utils::ErrorCode bind(int fd, const INetEndPoint& endpoint) const;

			light::utils::ErrorCode connect(int fd, const INetEndPoint& endpoint) const;

			light::utils::ErrorCode listen(int fd, int backlog/*=5*/) const;

			virtual light::utils::ErrorCode close(int &fd) const;

		};

		class TcpSocketOps : public SocketOps {/*{{{*/
		public:
			light::utils::ErrorCode open(int &sockfd, const protocol::V4& v4) const noexcept;

			light::utils::ErrorCode open(int &sockfd, const protocol::V6& v6) const noexcept;

		};/*}}}*/

		class UdpSocketOps : public SocketOps {
		public:
			light::utils::ErrorCode open(int &sockfd, const protocol::V4& v4) const noexcept;

			light::utils::ErrorCode open(int &sockfd, const protocol::V6& v6) const noexcept;
		};

		class Socket : public light::utils::NonCopyable {
		public:

			static TcpSocketOps& tcp_ops();

			static UdpSocketOps& udp_ops();

			virtual ~Socket() {}
			Socket(const SocketOps &ops): sockfd_(0), ops_(&ops) {}
			Socket(const SocketOps &ops, int fd): sockfd_(fd), ops_(&ops) {}

			explicit Socket(const SocketOps &ops, const INetEndPoint& endpoint);

			virtual light::utils::ErrorCode set_nonblocking();

			virtual light::utils::ErrorCode set_reuseaddr(int enable);

			virtual light::utils::ErrorCode get_last_error() { return light::utils::check_socket_error(sockfd_); }

			virtual light::utils::ErrorCode bind(const INetEndPoint& endpoint) {return ops_->bind(sockfd_, endpoint);}
			virtual light::utils::ErrorCode listen(int backlog=5) {return ops_->listen(sockfd_, backlog);}
			virtual light::utils::ErrorCode connect(const INetEndPoint& endpoint) {return ops_->connect(sockfd_, endpoint);}


			light::utils::ErrorCode open(const protocol::V4& v4) noexcept {return ops_->open(this->sockfd_, v4);}
			light::utils::ErrorCode open(const protocol::V6& v6) noexcept {return ops_->open(this->sockfd_, v6);}

			light::utils::ErrorCode open(const protocol::All& all) noexcept;

			light::utils::ErrorCode open(const INetEndPoint &endpoint) noexcept {return ops_->open_and_bind(this->sockfd_, endpoint);}

			virtual light::utils::ErrorCode close() { return ops_->close(this->sockfd_); }

			virtual ssize_t write(light::utils::ErrorCode &ec, const void *buf, size_t len, int flags=0);

			virtual ssize_t read(light::utils::ErrorCode &ec, void *buf, size_t len, int flags=0);

			inline int get_sockfd() const { return sockfd_; }

			light::utils::ErrorCode get_local_endpoint(INetEndPoint& endpoint);

		protected:
			int sockfd_;
			INetEndPoint local_point_;
			const SocketOps *ops_;
		};


		class TcpSocket : public Socket {
		public:

			TcpSocket(): Socket(Socket::tcp_ops()) {}
			TcpSocket(int fd): Socket(Socket::tcp_ops(), fd) {}
			explicit TcpSocket(const INetEndPoint& endpoint): Socket(Socket::tcp_ops(), endpoint) {}

			light::utils::ErrorCode accept(TcpSocket& client_socket);
			light::utils::ErrorCode accept(int& fd);

			light::utils::ErrorCode set_keepalive();
		};

		class UdpSocket : public Socket {
		public:
			UdpSocket(): Socket(Socket::udp_ops()) {}
			UdpSocket(int fd): Socket(Socket::udp_ops(), fd) {}
			explicit UdpSocket(const INetEndPoint& endpoint): Socket(Socket::udp_ops(), endpoint) {}

		};
	} /* network */
}
