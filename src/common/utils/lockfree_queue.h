#pragma once
#include "utils/platform.h"
namespace light {
	namespace utils {

#if 0
		template<typename T>
			struct LockFreeQueueNode {

				LockFreeQueueNode(T&& t): next(nullptr), data(std::forward<T>(t)) {}

				LockFreeQueueNode next;
				T data;
			};

		template<typename T>
			class LockFreeQueue {
			public:
				LockFreeQueue(): head_(nullptr), tail_(nullptr) {}

				bool empty() {
					return head_ == nullptr;
				}

				void push_back(T&& t) {
					LockFreeQueueNode<T> *node = new LockFreeQueueNode(std::forward<T>(t));
					if (compare_and_swap(tail_, nullptr, node)) {
						head_ = node;
						return;
					}

					LockFreeQueueNode<T> *old_tail = tail_.load(std::memory_order_release);
					LockFreeQueueNode<T> *p = old_tail;
					do {
						while (p->next != nullptr) {
							p = p->next;
						}
					} while(compare_and_swap(p->next, nullptr, node));
				}

				T front() {
				}

			private:
				LockFreeQueueNode<T>* head_;
				LockFreeQueueNode<T>* tail_;
			};

#endif
	} /* utils */
} /* light */
