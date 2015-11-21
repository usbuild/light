#pragma once
#ifdef HAVE_KQUEUE

#include "network/poller.h"
#include "utils/helpers.h"

namespace light {
	namespace network {

#define MAX_LOOPER_EVENTS 20

		class KqueuePoller : public Poller {
		public:
			KqueuePoller(Looper &looper);

			virtual ~KqueuePoller();

			light::utils::ErrorCode poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers);
			
			light::utils::ErrorCode add_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode remove_dispatcher(Dispatcher &dispatcher);

			light::utils::ErrorCode update_dispatcher(Dispatcher &dispatcher);

		private:
			int kqueuefd_;
		};
		
	} /* network */
} /* light */

#endif
