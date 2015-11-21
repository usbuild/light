#pragma once
#include <unordered_map>
#include "network/dispatcher.h"
#include "utils/noncopyable.h"
namespace light {
	namespace network {
		class Looper;

		class Poller : light::utils::NonCopyable {
		public:
			Poller(Looper &looper):dispatchers_(), looper_(&looper) {UNUSED(looper_);}

			virtual ~Poller();

			virtual light::utils::ErrorCode poll(int timeout, std::unordered_map<int, Dispatcher*> &active_dispatchers) = 0;

			virtual light::utils::ErrorCode add_dispatcher(Dispatcher &dispatcher) = 0;

			virtual light::utils::ErrorCode remove_dispatcher(Dispatcher &dispatcher) = 0;

			virtual light::utils::ErrorCode update_dispatcher(Dispatcher &dispatcher) = 0;

			bool has_dispathcer(const Dispatcher &dispatcher) const;

			static Poller* create_default_poller(Looper &looper);

		protected:
			std::unordered_map<int, Dispatcher*> dispatchers_;

		private:
			Looper *looper_;
		};

	} /* network */
} /* light */
