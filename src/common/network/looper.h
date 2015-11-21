#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <list>
#ifdef HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#ifdef HAVE_TIMERFD
#include <sys/timerfd.h>
#endif

#include "network/poller.h"
#include "network/timer.h"
namespace light {
	namespace network {

		struct RawMessage {
			RawMessage(): data(nullptr), length(0) {}
			RawMessage(const void *d, size_t l): data(d), length(l) {}
			const void *data;
			size_t length;
		};

		class Looper;

		template<typename FUNC>
		class SafeCallWrapper {
		public:
			SafeCallWrapper(Looper &looper, FUNC func, int strand_id): looper_(&looper), func_(func), strand_id_(strand_id) {}

			template<typename... ARGS>
				void operator()(ARGS&&... args);

		private:
			Looper *looper_;
			FUNC func_;
			int strand_id_;
		};

		class Strand {
		public:
			Strand(Looper &looper, int strand_id): looper_(&looper), strand_id_(strand_id) {}

			template<typename FUNC, typename... ARGS>
				void post(FUNC func, ARGS&&... args);

			template<typename FUNC>
				SafeCallWrapper<FUNC> wrap(FUNC func);

		private:
				Looper *looper_;
				int strand_id_;
		};


		class Looper {
		public:
			typedef std::function<void(RawMessage*)> message_handler_t;
			typedef std::function<void()> loop_callback_t;
			typedef std::lock_guard<std::mutex> lock_guard_t;
		public:
			Looper();


			void loop();

			light::utils::ErrorCode add_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode remove_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode update_dispatcher(Dispatcher &dispatcher);

			Dispatcher& get_time_dispatcher();

			Dispatcher& get_event_dispatcher();

			uintptr_t add_timer(light::utils::ErrorCode &ec, Timestamp timeout, Timestamp interval, const functor &time_callback);

			void cancel_timer(light::utils::ErrorCode &ec, const uintptr_t timer_id);

			light::utils::ErrorCode update_timerfd_expire();

			void stop();

			int register_loop_callback(const loop_callback_t &func, int idx=-1);
			void unregister_loop_callback(int idx);

			/**
			 * @brief 提供给非loop线程调用本类管理对象中方法的接口，注意，任何其它线程只能调用
			 * 这个接口，不允许调用本类中的其它方法
			 *
			 * @tparam FUNC 调用的方法
			 * @tparam ARGS 方法参数
			 * @param func 调用的方法
			 * @param args 方法参数
			 */
			template<typename FUNC, typename... ARGS>
				void post(FUNC func, ARGS&&... args) {
					strand_post(0, func, std::forward<ARGS>(args)...);
				}

			/**
			 * @brief 类似于 Looper::post() ，但是提供了一个strand_id用于保证同一时间只有只能有一个线程
			 * 执行该strand_id对应的函数。
			 *
			 * @tparam FUNC
			 * @tparam ARGS
			 * @param strand_id 如果为0的话则表示不做限制
			 * @param func
			 * @param args
			 */
			template<typename FUNC, typename... ARGS>
				void strand_post(int strand_id, FUNC func, ARGS&&... args) {
					lock_guard_t lock(post_functor_lock_);
					uint64_t data = 1;
#ifdef HAVE_EVENTFD
					int ret = ::write(event_dispatcher_->get_fd(), &data, sizeof data);
#else
					int ret = ::write(w_event_dispatcher_->get_fd(), &data, sizeof data);
#endif
					if (strand_id) {
						unique_post_functors_[strand_id].push_back(std::bind(func, std::forward<ARGS>(args)...));
					} else {
						post_functors_.push_back( std::bind(func, std::forward<ARGS>(args)...));
					}
				}

			/**
			 * @brief 将本线程中的方法包装一下，提供给别的线程回调
			 *
			 * @tparam FUNC 调用的方法
			 * @param func 调用的方法
			 *
			 * @return 一个将在本线程中调用的回调函数
			 */
			template<typename FUNC>
			SafeCallWrapper<FUNC> wrap(FUNC func) {
				return strand_wrap(func, 0);
			}

			template<typename FUNC>
				SafeCallWrapper<FUNC> strand_wrap(FUNC func, int strand_id) {
					return SafeCallWrapper<FUNC>(*this, func, strand_id);
				}

			Strand get_strand(int strand_id) {
				return Strand(*this, strand_id);
			}

		private:
			void functors_work();

			void tick_timer();

		private:
			std::unique_ptr<Poller> poller_;
			std::unique_ptr<Dispatcher> timer_dispatcher_;
			std::unique_ptr<Dispatcher> event_dispatcher_;
#ifndef HAVE_EVENTFD
			std::unique_ptr<Dispatcher> w_event_dispatcher_;
#endif
			bool stop_;
			std::unordered_map<int, Dispatcher*> valid_dispatchers_;
			TimerQueue queue_;

			std::map<int, loop_callback_t> loop_callbacks_;
			int last_callback_idx_;

			//may change to lock free queue
			std::list<functor> post_functors_;

			std::atomic_int running_workers_;
			std::mutex cond_lock_;
			std::condition_variable cond_var_;
			int notify_valid_;

			std::map<int, std::vector<functor> > unique_post_functors_;
			std::mutex post_functor_lock_;
			std::set<int> running_post_queue_ids_;
		};

		template<typename FUNC>
		template<typename... ARGS>
			void SafeCallWrapper<FUNC>::operator()(ARGS&&... args) {
				looper_->strand_post(strand_id_, func_, std::forward<ARGS>(args)...);
			}

		template<typename FUNC, typename... ARGS>
			void Strand::post(FUNC func, ARGS&&... args) {
				looper_->strand_post(strand_id_, func, std::forward<ARGS>(args)...);
			}

		template<typename FUNC>
			SafeCallWrapper<FUNC> Strand::wrap(FUNC func) {
				looper_->strand_wrap(strand_id_, func);
			}


	} /* network */
} /* light */
