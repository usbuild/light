#pragma once
#include <algorithm>
#include <set>
#include <time.h>
#include <tuple>
#include "utils/helpers.h"
#include "utils/logger.h"
#include <unordered_map>

namespace light {
	namespace network {

		typedef uint64_t Timestamp;
		typedef uint32_t TimerId;
		typedef std::function<void(void)> functor;

		class Timer : public std::enable_shared_from_this<Timer> {
		public:

			explicit Timer(Timestamp &tv) { Timer(tv, functor()); }

			Timer(Timestamp tv, functor expire_callback);

			Timer(Timestamp tv, Timestamp interval, functor expire_callback);

			bool expired(const Timestamp *now=nullptr) const;

			bool forward();

			inline bool repeatable() const { return interval_ != 0; }

			inline bool triggered() const { return triggered_; }

			Timestamp &get_next() { return this->next_; }

			bool operator<(const Timer &rhs) const {
				return (next_ < rhs.next_) || (next_ == rhs.next_ && this < &rhs);
			}

			void set_timer_id(TimerId timer_id)
			{
				timer_id_ = timer_id;
			}

			TimerId get_timer_id() const
			{
				return timer_id_;
			}
		private:
			Timestamp next_;
			Timestamp interval_;
			functor expire_callback_;
			bool triggered_;
			TimerId timer_id_;
		};

		class TimerQueue : public light::utils::NonCopyable{
		public:
			TimerQueue(): queue_() {}

			TimerId add_timer(Timestamp expire_time, Timestamp interval, bool &need_update, functor expire_callback);

			void del_timer(TimerId timer_id, bool &need_update);

			void print_queue();

			void update_time(Timestamp now);

			void update_time_now();

			std::shared_ptr<Timer> get_nearist_timer();

			bool insert(std::shared_ptr<Timer> timer);

		private:
			typedef std::pair<Timestamp, std::shared_ptr<Timer> > TimerEntry;

			std::unordered_map<TimerId, std::set<TimerEntry>::const_iterator> timer_id_map_;
			std::set<TimerEntry> queue_;

			TimerId last_insert_timer_id_ = 0;
		};
	} /* network */
} /* light */
