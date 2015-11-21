#pragma once
#ifdef HAVE_EPOLL
#include <sys/epoll.h>
#include "network/poller.h"
#include "network/epoll_poller.h"
#include "utils/exception.h"
#include "utils/helpers.h"

namespace light {
	namespace network {

#define MAX_LOOPER_EVENTS 20

		class EpollPoller : public Poller {
		public:
			EpollPoller(Looper &looper);

			virtual ~EpollPoller();

			light::utils::ErrorCode poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers);
			
			light::utils::ErrorCode add_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode remove_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode update_dispatcher(Dispatcher &dispatcher);

		private:
			struct epoll_event events_[MAX_LOOPER_EVENTS];
			int epollfd_;
		};
		
	} /* network */
} /* light */
#endif
