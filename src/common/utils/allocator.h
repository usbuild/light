#pragma once
#include <mutex>

namespace light {
	namespace utils {

		template<int N>
			union FixedMemNode {
				FixedMemNode(): next(nullptr) {}
				FixedMemNode *next;
				char data[N];
			};
		template<int N, int MAX_RESERVE=5>
			class FixedAllocator {
			public:
			public:
				FixedAllocator(): free_list_(nullptr), free_count_(0) {}
				char *alloc() {
					std::lock_guard<std::mutex> lock_guard(lock_);
					if (free_list_ == nullptr) {
						free_list_ = new FixedMemNode<N>();
						++free_count_;
					}
					char *d = free_list_->data;
					free_list_ = free_list_->next;
					--free_count_;
					return d;
				}

				void free(char *data) {
					std::lock_guard<std::mutex> lock_guard(lock_);
					FixedMemNode<N> *node = reinterpret_cast<FixedMemNode<N>*>(data);
					node->next = free_list_;
					free_list_ = node;
					++free_count_;
					while (free_count_ >= MAX_RESERVE) {
						auto *tmp_node = free_list_->data;
						free_list_ = free_list_->next;
						delete tmp_node;
						--free_list_;
					}
				}

				constexpr int node_size() const {return N;}
			private:
				FixedMemNode<N> *free_list_;
				std::mutex lock_;
				uint32_t free_count_;
			};
	} /* ut */
} /* light */
