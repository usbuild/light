#pragma once
#include <algorithm>
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>

namespace light {
	namespace utils {

		class RWBuffer;
		class Buffer {
		public:
			Buffer():data_(nullptr), len_(0), capacity_(0), alloced_(false){}
			~Buffer() {
				if (capacity_ && alloced_) {
					free(data_);
				}
			}

			void resize(size_t sz) {
				this->data_ = ::realloc(this->data_, sz);
				this->capacity_ = sz;
				alloced_ = true;
			}

			Buffer& append(const void *data, size_t len) {
				if (capacity_ == 0) {
					this->resize(len);
				} else if (capacity_ < len_ + len) {
					this->resize(std::max(len_ + len, static_cast<size_t>(capacity_ * 1.5)));
				}
				memcpy(static_cast<char*>(this->data_) + len_, data, len);
				len_ += len;
				return *this;
			}

			size_t size() const {
				return len_;
			}

			size_t capacity() const {
				return capacity_;
			}

			void* data() const {
				return data_;
			}

			void clear() {
				len_ = 0;
			}
		
		protected:
			void *data_;
			size_t len_;
			size_t capacity_;
			bool alloced_;
		};

		class RWBuffer :public Buffer {
		public:
			RWBuffer() : Buffer(), read_(0) {}
			void* read(int bytes) {
				if (bytes + read_ > len_) {
					return nullptr;
				}
				void* ptr = static_cast<char*>(data_) + read_;
				read_ += bytes;
				return ptr;
			}
			void rewind() {
				read_ = 0;
			}
		private:
			size_t read_;
		};

	} /* utils */
} /* light */
