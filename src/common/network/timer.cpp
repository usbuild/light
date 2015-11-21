#include "network/timer.h"
namespace light {
	namespace network {
		Timer::Timer(Timestamp tv, functor expire_callback):
			next_(tv),
			interval_(0) ,
			expire_callback_(expire_callback), triggered_(false) {}

		Timer::Timer(Timestamp tv, Timestamp interval, functor expire_callback):
			next_(tv),
			interval_(interval),
			expire_callback_(expire_callback), triggered_(false) {}

		bool Timer::expired(const Timestamp *now) const {
			Timestamp val;
			if (now) {
				val = *now;
			} else {
				val = light::utils::get_timestamp();
			}
			return next_ < val;
		}

		bool Timer::forward() {
			if (!expired()) return false;
			if (triggered_) return false;
			triggered_ = true;
			if (expire_callback_) expire_callback_();
			if (repeatable()) {
				triggered_ = false;
				next_ += interval_;
			}
			return true;
		}


		//TimerQueue
		Timer* TimerQueue::add_timer(Timestamp expire_time, Timestamp interval, bool &need_update, functor expire_callback) {
			auto now = light::utils::get_timestamp();
			auto timer = new Timer(expire_time + now, interval, expire_callback);
			need_update = insert(*timer);
			return timer;
		}

		void TimerQueue::del_timer(Timer *timer, bool &need_update) {
			need_update = false;
			TimerEntry ent(timer->get_next(), timer);
			auto it = queue_.find(ent);
			if (it != queue_.end()) {
				if (it == queue_.begin()) need_update = true;
				queue_.erase(TimerEntry(timer->get_next(), timer));
				delete timer;
			}
		}

		void TimerQueue::print_queue() {
			LOG(DEBUG) << "QUEUE " << queue_.size();
			for (auto &kv : queue_) {
				LOG(DEBUG) << "next: " << kv.first << "  timer: " << kv.second;
			}
		}

		void TimerQueue::update_time(Timestamp now) {
			for (auto it = queue_.begin(); it != queue_.end(); ) {
				auto timer = it->second;
				if (timer->expired(&now)) {
					timer->forward();
					if (queue_.begin() == queue_.end() || queue_.begin()->second != timer) {
						//this iterator is invalid, (may cancelled )reset to begin
						it = queue_.begin();
						continue;
					}
					it = queue_.erase(it);
					if (timer->repeatable()) {
						queue_.insert(TimerEntry(timer->get_next(), timer));
						it = queue_.begin();
					} else {
						delete timer;
					}
				} else {
					break;
				}
			}
		}

		void TimerQueue::update_time_now() {
			update_time(light::utils::get_timestamp());
		}

		Timer* TimerQueue::get_nearist_timer() {
			if (queue_.size()) {
				return queue_.begin()->second;
			}
			return nullptr;
		}

		bool TimerQueue::insert(Timer &timer) {
			bool need_update = false;
			auto next = timer.get_next();
			auto it = queue_.begin();
			if (it == queue_.end() || next < it->first) {
				need_update = true;
			}
			queue_.insert(TimerEntry(timer.get_next(), &timer));
			return need_update;
		}
	} /* ne */

} /* light */
