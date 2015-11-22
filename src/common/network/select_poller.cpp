#if !defined(HAVE_EPOLL_H) && !defined(HAVE_KQUEUE_H)

#include <sys/types.h>

#include "network/select_poller.h"
#include "utils/exception.h"

namespace light {
	namespace network {

		SelectPoller::SelectPoller(Looper &looper) : Poller(looper) {
			FD_ZERO(&readfds_);
			FD_ZERO(&writefds_);
			FD_ZERO(&errorfds_);
		}

		SelectPoller::~SelectPoller() {}

		//millisecs
		light::utils::ErrorCode SelectPoller::poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers) {


			struct timeval tv;
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = timeout % 1000 * 1000000;
			
			int num_events = ::select(0/*compatible for win*/, &readfds_, &writefds_, &errorfds_, &tv);

			if (num_events < 0) {
				if (errno == EINTR) {
					return LS_OK_ERROR();
				}
			} else if (num_events == 0) {
				return LS_OK_ERROR();
			} else {

				for (auto &kv : dispatchers_)
				{
					bool valid = true;
					if (FD_ISSET(kv.first, &readfds_))
					{
						kv.second->set_poll_event_data(true, false, false, false);
					} else if (FD_ISSET(kv.first, &writefds_))
					{
						kv.second->set_poll_event_data(false, true, false, false);
					} else if (FD_ISSET(kv.first, &errorfds_))
					{
						kv.second->set_poll_event_data(false, false, true, false);
					} else
					{
						valid = false;
					}
					if (valid)
					{
						active_dispatchers[kv.first] = kv.second;
					}
				}
			}
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode SelectPoller::add_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) == dispatchers_.end());
			int sock = dispatcher.get_fd();
			
			if (dispatcher.readable()) {
				FD_SET(sock, &readfds_);
			}
			if (dispatcher.writable()) {
				FD_SET(sock, &writefds_);
			}
			FD_SET(sock, &errorfds_);

			dispatchers_[dispatcher.get_fd()] = &dispatcher;
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode SelectPoller::remove_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());

			int sock = dispatcher.get_fd();
			FD_CLR(sock, &readfds_);
			FD_CLR(sock, &writefds_);
			FD_CLR(sock, &errorfds_);

			dispatchers_.erase(dispatcher.get_fd());
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode SelectPoller::update_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());
			int sock = dispatcher.get_fd();
			
			if (dispatcher.readable()) {
				FD_SET(sock, &readfds_);
			} else {
				FD_CLR(sock, &readfds_);
			}
			if (dispatcher.writable()) {
				FD_SET(sock, &writefds_);
			} else {
				FD_CLR(sock, &writefds_);
			}
			FD_SET(sock, &errorfds_);
			return LS_OK_ERROR();
		}

	} /* network */
} /* light */

#endif
