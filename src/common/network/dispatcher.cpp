#include "network/dispatcher.h"
#include "network/looper.h"

namespace light {
	namespace network {

		light::utils::ErrorCode Dispatcher::attach(Looper& looper) {
			if (attached_) return LS_OK_ERROR();
			this->looper_ = &looper;
			this->attached_ = true;
			return looper_->add_dispatcher(*this);
		}
		light::utils::ErrorCode Dispatcher::detach() {
			if (!attached_) return LS_OK_ERROR();
			this->attached_ = false;
			return looper_->remove_dispatcher(*this);
		}

		light::utils::ErrorCode Dispatcher::reattach() {
			if (attached_) {
				return looper_->update_dispatcher(*this);
			} else {
				return attach();
			}
		}

		light::utils::ErrorCode Dispatcher::attach() {
			return attach(*looper_);
		}
		void Dispatcher::set_poll_event_data(bool r, bool w, bool e, bool c) {
			poll_events_.read = r;
			poll_events_.write = w;
			poll_events_.error = e;
			poll_events_.close = c;
		}

		void Dispatcher::handle_events() {
			//分次处理，防止前面的callback对后面的操作有干扰
			if (poll_events_.read) {
				if (read_callback_) read_callback_();
			}
			else if (poll_events_.close) {
				if (close_callback_) close_callback_();
			}
			else if (poll_events_.error) {
				if (error_callback_) error_callback_();
			}
			else if (poll_events_.write) {
				if (write_callback_) write_callback_();
			}
		}

		const int Dispatcher::NO_EVENT = 0;
		const int Dispatcher::READ_EVENT = (1 << 1);
		const int Dispatcher::WRITE_EVENT (1 << 2);

	} /* network */
} /* light */
