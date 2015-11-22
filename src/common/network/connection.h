#pragma once
#include <functional>
#include <memory>
#include <stdint.h>
#include <vector>
#include "utils/platform.h"

namespace light {
	namespace network {

		class Looper;

		typedef std::function<void()> write_callback_t;
		struct WriteBufferNode {
			WriteBufferNode(void* buf, size_t len, const write_callback_t& func): buffer(buf), total_len(len), write_ptr(0), callback(func) {}
			void *buffer;
			size_t total_len;
			size_t write_ptr;
			write_callback_t callback;
		};

		class WriteBuffer {
		public:
			WriteBuffer();
			inline bool empty() {return size() == 0;}
			inline size_t size() {return size_;}
			
			void append(void* buffer, size_t len, const write_callback_t &callback);

			std::vector<struct iovec>&  get_iovec(size_t max_size=SIZE_MAX);

			void shift(size_t len);

		private:
			std::deque<WriteBufferNode> write_queue_;
			size_t size_;
			std::vector<struct iovec> write_vect_;
		};

		class Connection : public std::enable_shared_from_this<Connection>{
		public:
			Connection(Looper &looper): looper_(&looper) {}
		protected:
			Looper *looper_;
		};
		
	} /* network */
} /* light */
