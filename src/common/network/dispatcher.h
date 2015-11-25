#pragma once

#include <vector>
#include <memory>
#include <map>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "network/socket.h"
#include "utils/error_code.h"
#include "utils/logger.h"
#include "utils/noncopyable.h"

namespace light {
	namespace network {

		class Looper;

		struct PollEventData {
			PollEventData(bool r, bool w, bool e, bool c): read(r), write(w), error(e), close(c) {}
			bool read;
			bool write;
			bool error;
			bool close;
		};

		class Dispatcher: public light::utils::NonCopyable, public std::enable_shared_from_this<Dispatcher>  {

		public:
			typedef std::function<void()> event_callback;

		public:
			Dispatcher(Looper &looper, int fd): looper_(&looper), attached_(false), events_(NO_EVENT), fd_(fd),
			read_callback_(),
			write_callback_(),
			close_callback_(),
			error_callback_(),
			poll_events_(false, false, false, false)
			{
			}

			~Dispatcher() {
				if (attached_) detach();
			}

		public:
			void enable_read() {events_ |= READ_EVENT;reattach();}
			void enable_write() {events_ |= WRITE_EVENT;reattach();}
			void disable_read() {events_ &= ~READ_EVENT;reattach();}
			void disable_write() {events_ &= ~WRITE_EVENT;reattach();}
			void disable_all() {events_ = NO_EVENT;reattach();}

			bool readable() {return events_ & READ_EVENT;}
			bool writable() {return events_ & WRITE_EVENT;}


			void set_read_callback(const event_callback &cb) { read_callback_.set_callback(cb); }
			void set_write_callback(const event_callback &cb) { write_callback_.set_callback(cb); }
			void set_close_callback(const event_callback &cb) { close_callback_.set_callback(cb); }
			void set_error_callback(const event_callback &cb) { error_callback_.set_callback(cb); }

			void set_read_callback(event_callback &&cb) { read_callback_.set_callback(cb); }
			void set_write_callback(event_callback &&cb) { write_callback_.set_callback(cb); }
			void set_close_callback(event_callback &&cb) { close_callback_.set_callback(cb); }
			void set_error_callback(event_callback &&cb) { error_callback_.set_callback(cb); }


			void set_poll_event_data(bool r, bool w, bool e, bool c);

			void handle_events();

			int get_fd() const {
				return fd_;
			}

			inline bool attached() {return attached_;}

			inline Looper& get_looper() const {
				assert(looper_);
				return *looper_;
			}

			inline void set_looper(Looper& looper) {
				assert(!attached_);
				this->looper_ = &looper;
			}

			int get_index() const
			{
				return index_;
			}

			void set_index(int index)
			{
				index_ = index;
			}

		public:
			light::utils::ErrorCode attach(Looper &looper);
			light::utils::ErrorCode detach();
			light::utils::ErrorCode attach();
			light::utils::ErrorCode reattach();

		protected:
			class DispatcherEventCallback {
			public:
				DispatcherEventCallback(): running_idx_(0), next_idx_(0) {}

				DispatcherEventCallback(const std::function<void()> &cb): running_idx_(0), next_idx_(0) {
					set_callback(cb);
				}

				DispatcherEventCallback(std::function<void()> &&cb): running_idx_(0), next_idx_(0) {
					set_callback(cb);
				}

				explicit operator bool() const {
					return !!cb_[next_idx_];
				}

				void operator()() {
					running_idx_ = next_idx_;
					cb_[next_idx_]();
				}

				void set_callback(const event_callback &cb) {
					next_idx_ = 1 - running_idx_;
					cb_[next_idx_] = cb;
				}

				void set_callback(event_callback &&cb) {
					next_idx_ = 1 - running_idx_;
					cb_[next_idx_] = cb;
				}


			private:
				event_callback cb_[2];
				uint8_t running_idx_;
				uint8_t next_idx_;
			};


			Looper* looper_;
			bool attached_;
			int events_;
			int fd_;

			DispatcherEventCallback read_callback_;
			DispatcherEventCallback write_callback_;
			DispatcherEventCallback close_callback_;
			DispatcherEventCallback error_callback_;

			PollEventData poll_events_;
			int index_;
			static const int NO_EVENT;
			static const int READ_EVENT;
			static const int WRITE_EVENT;
		};

	} /* network */
} /* light */

