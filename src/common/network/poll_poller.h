#pragma once
#include "config.h"
#if !defined(HAVE_EPOLL_H) && !defined(HAVE_KQUEUE_H)

#include "network/poller.h"
#include "utils/helpers.h"

namespace light {
namespace network {

#define MAX_LOOPER_EVENTS 20

class PollPoller : public Poller {
public:
  PollPoller(Looper &looper);

  virtual ~PollPoller();

  std::error_code
  poll(int timeout, std::unordered_map<int, Dispatcher *> &active_dispatchers);

  std::error_code add_dispatcher(Dispatcher &dispatcher);

  std::error_code remove_dispatcher(Dispatcher &dispatcher);

  std::error_code update_dispatcher(Dispatcher &dispatcher);

private:
  std::vector<struct pollfd> fds_;
};

} /* network */
} /* light */

#endif
