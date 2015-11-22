#include "config.h"
#ifdef HAVE_EPOLL_H
#include "network/epoll_poller.h"
namespace light {
	namespace network {

		EpollPoller::EpollPoller(Looper &looper) : Poller(looper), epollfd_(0) {
			if ((this->epollfd_ = epoll_create1(EPOLL_CLOEXEC)) == -1) {
				this->epollfd_ = 0;
				throw light::exception::EventException(LS_GENERIC_ERROR(errno));
			}
		}

		EpollPoller::~EpollPoller() {}

		light::utils::ErrorCode EpollPoller::poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers) {
			int num_events = ::epoll_wait(epollfd_, events_, MAX_LOOPER_EVENTS, timeout);
			if (num_events < 0) {
				if (errno == EINTR) {
					return LS_OK_ERROR();
				}
			} else if (num_events == 0) {
				return LS_OK_ERROR();
			} else {
				for (int i = 0; i < num_events; ++i) {
					auto dispatcher = static_cast<Dispatcher*>(events_[i].data.ptr);
					assert(dispatchers_.find(dispatcher->get_fd()) != dispatchers_.end());
					auto event = events_[i].events;
					dispatcher->set_poll_event_data(
						event & (EPOLLIN | EPOLLPRI | EPOLLRDHUP),
						event & EPOLLOUT,
						event & EPOLLERR,
						(event & EPOLLHUP) && !(event & EPOLLIN)
						);

					/*
					DLOG(INFO) << "fd: " << dispatcher->get_fd() << " events: " << bool(event & EPOLLIN) << " " << bool(event & EPOLLPRI) << " " << bool(event & EPOLLOUT) <<
						" " << bool(event & EPOLLERR) << " " << bool(event & EPOLLHUP) << " " << bool(event & EPOLLRDHUP);
						*/
					active_dispatchers[dispatcher->get_fd()] = dispatcher;
				}
			}
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode EpollPoller::add_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) == dispatchers_.end());

			struct epoll_event ev;
			ev.events = 0;
			if (dispatcher.readable()) ev.events |= EPOLLIN | EPOLLPRI;
			if (dispatcher.writable()) ev.events |= EPOLLOUT;
			ev.data.ptr = &dispatcher;
			if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, dispatcher.get_fd(), &ev) == -1) {
				return LS_GENERIC_ERROR(errno);
			}
			dispatchers_[dispatcher.get_fd()] = &dispatcher;
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode EpollPoller::remove_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());

			struct epoll_event ev;
			if(::epoll_ctl(epollfd_, EPOLL_CTL_DEL, dispatcher.get_fd(), &ev) == -1) {
				return LS_GENERIC_ERROR(errno);
			}
			dispatchers_.erase(dispatcher.get_fd());
			return LS_OK_ERROR();
		}

		light::utils::ErrorCode EpollPoller::update_dispatcher(Dispatcher &dispatcher) {
			assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());
			struct epoll_event ev;
			ev.events = 0;
			if (dispatcher.readable()) ev.events |= EPOLLIN | EPOLLPRI;
			if (dispatcher.writable()) ev.events |= EPOLLOUT;
			ev.data.ptr = &dispatcher;
			if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, dispatcher.get_fd(), &ev) == -1) {
				return LS_GENERIC_ERROR(errno);
			}
			return LS_OK_ERROR();
		}

	} /* network */
} /* light */
#endif
