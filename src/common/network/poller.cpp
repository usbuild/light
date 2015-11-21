#include "network/poller.h"
#include "network/epoll_poller.h"
#include "network/kqueue_poller.h"
namespace light {
	namespace network {
		Poller::~Poller() {}

		Poller* Poller::create_default_poller(Looper &looper) {
#ifdef HAVE_EPOLL
			return new EpollPoller(looper);
#endif
#ifdef HAVE_KQUEUE
			return new KqueuePoller(looper);
#endif
		}

		bool Poller::has_dispathcer(const Dispatcher &dispatcher) const {
			auto res = dispatchers_.find(dispatcher.get_fd());
			return res != dispatchers_.end() && &dispatcher == res->second;
		}
	} /* network */
} /* light */
