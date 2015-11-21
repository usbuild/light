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
			 * @brief �ṩ����loop�̵߳��ñ����������з����Ľӿڣ�ע�⣬�κ������߳�ֻ�ܵ���
			 * ����ӿڣ���������ñ����е���������
			 *
			 * @tparam FUNC ���õķ���
			 * @tparam ARGS ��������
			 * @param func ���õķ���
			 * @param args ��������
			 */
			template<typename FUNC, typename... ARGS>
				void post(FUNC func, ARGS&&... args) {
					strand_post(0, func, std::forward<ARGS>(args)...);
				}

			/**
			 * @brief ������ Looper::post() �������ṩ��һ��strand_id���ڱ�֤ͬһʱ��ֻ��ֻ����һ���߳�
			 * ִ�и�strand_id��Ӧ�ĺ�����
			 *
			 * @tparam FUNC
			 * @tparam ARGS
			 * @param strand_id ���Ϊ0�Ļ����ʾ��������
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
			 * @brief �����߳��еķ�����װһ�£��ṩ������̻߳ص�
			 *
			 * @tparam FUNC ���õķ���
			 * @param func ���õķ���
			 *
			 * @return һ�����ڱ��߳��е��õĻص�����
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
