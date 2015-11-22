#include <assert.h>
#include <vector>
#include <deque>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#include "network/connection.h"

namespace light {
	namespace network {
		WriteBuffer::WriteBuffer(): write_queue_(), size_(0), write_vect_() {}

		void WriteBuffer::append(void* buffer, size_t len, const write_callback_t &func) {
			write_queue_.emplace_back(WriteBufferNode(buffer, len, func));
			size_ += len;
		}

		std::vector<struct iovec>&  WriteBuffer::get_iovec(size_t max_size) {
			write_vect_.clear();
			size_t current_size = 0;

			for (auto &n : write_queue_) {
				struct ::iovec v;
				auto data_len = n.total_len - n.write_ptr;
				if (current_size + data_len >= max_size) break;
				v.iov_base = static_cast<char*>(n.buffer) + n.write_ptr;
				v.iov_len = data_len;
				current_size += data_len;
				write_vect_.emplace_back(v);
			}
			return write_vect_;
		}

		void WriteBuffer::shift(size_t len) {
			assert(len <= size_);
			if (len == 0) return;
			size_t removed_len = 0;
			auto last_it = write_queue_.end();
			for (auto it = write_queue_.begin(); it != write_queue_.end(); ++it) {
				if (removed_len + it->total_len - it->write_ptr > len) {
					last_it = it;
					break;
				} else {
					removed_len += it->total_len;
					if (it->callback) it->callback();
				}
			}
			write_queue_.erase(write_queue_.begin(), last_it);
			if (write_queue_.begin() == write_queue_.end())
			{
				size_ = 0;
				return;
			}
			write_queue_.begin()->write_ptr += len - removed_len;
			size_ -= len;
		}
	} /* network */

} /* light */
