#ifdef HAVE_KQUEUE

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "network/kqueue_poller.h"
#include "utils/exception.h"

namespace light {
	namespace network {

		KqueuePoller::KqueuePoller(Looper &looper) : Poller(looper), kqueuefd_(0) {
			if ((this->kqueuefd_ = ::kqueue()) == -1) {
				this->kqueuefd_ = 0;
				throw light::exception::EventException(LS_GENERIC_ERROR(errno));
			}
		}

		KqueuePoller::~KqueuePoller() {}

		light::utils::ErrorCode KqueuePoller::poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers) {

			struct kevent ev[MAX_LOOPER_EVENTS];

			struct timespec spec;
			spec.tv_sec = timeout / 1000;
			spec.tv_nsec = timeout / 1000 * 1000000LL;
			int num_events = ::kevent(kqueuefd_, NULL, 0, ev, MAX_LOOPER_EVENTS, &spec);

			if (num_events < 0) {
				if (errno == EINTR) {
					return LS_OK_ERROR();
				}
			} else if (num_events == 0) {
				return LS_OK_ERROR();
			} else {
				for (int i = 0; i < num_events; ++i) {
					auto dispatcher = static_cast<Dispatcher*>(ev[i].udata);
					int filter = ev[i].filter;
					int flags = ev[i].flags;
					dispatcher->set_poll_event_data(
						filter == EVFILT_READ,
						filter == EVFILT_WRITE,
						flags & EV_ERROR,
						flags & EV_EOF
						);
					active_dispatchers[dispatcher->get_fd()] = dispatcher;
				}
			}
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode KqueuePoller::add_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) == dispatchers_.end());
			int sock = dispatcher.get_fd();
			struct kevent ke;
			if (dispatcher.readable()) {
				EV_SET(&ke, sock, EVFILT_READ, EV_ADD, 0, 0, &dispatcher);
				if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1) {
					return LS_GENERIC_ERROR(errno);
				}
			}
			if (dispatcher.writable()) {
				EV_SET(&ke, sock, EVFILT_WRITE, EV_ADD, 0, 0, &dispatcher);
				if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1) {
					EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
					if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1) {
						return LS_GENERIC_ERROR(errno);
					}
				}
			}

			dispatchers_[dispatcher.get_fd()] = &dispatcher;
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode KqueuePoller::remove_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());

			int sock = dispatcher.get_fd();
			struct kevent ke;
			EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
			if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1 && errno != ENOENT) {
				return LS_GENERIC_ERROR(errno);
			}
			EV_SET(&ke, sock, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1 && errno != ENOENT) {
				return LS_GENERIC_ERROR(errno);
			}

			dispatchers_.erase(dispatcher.get_fd());
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode KqueuePoller::update_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());
			int sock = dispatcher.get_fd();
			struct kevent ke;
			if (dispatcher.readable()) {
				EV_SET(&ke, sock, EVFILT_READ, EV_ADD, 0, 0, &dispatcher);
				if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1) {
					return LS_GENERIC_ERROR(errno);
				}
			} else {
				EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, &dispatcher);
				if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1 && errno != ENOENT) {
					return LS_GENERIC_ERROR(errno);
				}
			}
			if (dispatcher.writable()) {
				EV_SET(&ke, sock, EVFILT_WRITE, EV_ADD, 0, 0, &dispatcher);
				if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1) {
					EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
					if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1 && errno != ENOENT) {
						return LS_GENERIC_ERROR(errno);
					}
				}
			} else {
				EV_SET(&ke, sock, EVFILT_WRITE, EV_DELETE, 0, 0, &dispatcher);
				if (::kevent(kqueuefd_, &ke, 1, NULL, 0, NULL) == -1 && errno != ENOENT) {
					return LS_GENERIC_ERROR(errno);
				}
			}
			return LS_OK_ERROR();
		}

	} /* network */
} /* light */

#endif
