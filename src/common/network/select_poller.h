#pragma once
#include "config.h"
#if !defined(HAVE_EPOLL_H) && !defined(HAVE_KQUEUE_H)

#include "network/poller.h"
#include "utils/helpers.h"

namespace light {
	namespace network {

#define MAX_LOOPER_EVENTS 20

		class SelectPoller : public Poller {
		public:
			SelectPoller(Looper &looper);

			virtual ~SelectPoller();

			light::utils::ErrorCode poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers);
			
			light::utils::ErrorCode add_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode remove_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode update_dispatcher(Dispatcher &dispatcher);

		private:
			FD_SET readfds_;
			FD_SET writefds_;
			FD_SET errorfds_;
		};
		
	} /* network */
} /* light */

#endif
