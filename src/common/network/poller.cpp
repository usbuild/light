#include "network/poller.h"
#include "network/epoll_poller.h"
#include "network/kqueue_poller.h"
#include "network/poll_poller.h"
namespace light {
namespace network {
Poller::~Poller() {}

Poller *Poller::create_default_poller(Looper &looper) {

#ifdef HAVE_EPOLL_H
  return new EpollPoller(looper);
#elif defined(HAVE_KQUEUE_H)
  return new KqueuePoller(looper);
#else
  return new PollPoller(looper);
#endif
}

bool Poller::has_dispathcer(const Dispatcher &dispatcher) const {
  auto res = dispatchers_.find(dispatcher.get_fd());
  return res != dispatchers_.end() && &dispatcher == res->second;
}
} /* network */
} /* light */
