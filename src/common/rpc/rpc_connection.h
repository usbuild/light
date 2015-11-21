#pragma once

namespace light {
	namespace rpc {

		class Connection;

		class RawRpcMessage {
		public:
			RawRpcMessage(char *data, int length):data_(data), length_(length) { }

			RawRpcMessage(): data_(nullptr), length_(0) {}

		private:
			void *data_;
			size_t length_;
		};

		class RpcConnection : public TcpConnection{
		public:
				RpcConnection(Looper &looper): TcpConnection(looper) {}

				RpcConnection(Looper &looper, int fd): TcpConnection(looper, fd) {}
		private:
		};


	} /* rpc */
} /* light */
