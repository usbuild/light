#pragma once
#include "config.h"
#ifdef HAVE_EPOLL_H
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

  std::error_code
  poll(int timeout, std::unordered_map<int, Dispatcher *> &active_dispatchers);

  std::error_code add_dispatcher(Dispatcher &dispatcher);

  std::error_code remove_dispatcher(Dispatcher &dispatcher);

  std::error_code update_dispatcher(Dispatcher &dispatcher);

private:
  struct epoll_event events_[MAX_LOOPER_EVENTS];
  int epollfd_;
};

} /* network */
} /* light */
#endif
