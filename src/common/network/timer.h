#pragma once
#include <algorithm>
#include <set>
#include <time.h>
#include <tuple>
#include "utils/helpers.h"
#include "utils/logger.h"
namespace light {
	namespace network {

		typedef uint64_t Timestamp;
		typedef std::function<void(void)> functor;

		class Timer {
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

		private:
			Timestamp next_;
			Timestamp interval_;
			functor expire_callback_;
			bool triggered_;
		};

		class TimerQueue {
		public:
			TimerQueue(): queue_() {}

			Timer* add_timer(Timestamp expire_time, Timestamp interval, bool &need_update, functor expire_callback);

			void del_timer(Timer *timer, bool &need_update);

			void print_queue();

			void update_time(Timestamp now);

			void update_time_now();

			Timer* get_nearist_timer();

			bool insert(Timer &timer);

		private:
			typedef std::pair<Timestamp, Timer*> TimerEntry;
			std::set<TimerEntry> queue_;
		};
	} /* network */
} /* light */
