#include "config.h"
#if !defined(HAVE_EPOLL_H) && !defined(HAVE_KQUEUE_H)

#include <sys/types.h>

#include "network/poll_poller.h"
#include "utils/exception.h"

namespace light {
namespace network {

PollPoller::PollPoller(Looper &looper) : Poller(looper) {}

PollPoller::~PollPoller() {}

// millisecs
std::error_code
PollPoller::poll(int timeout,
                   std::unordered_map<int, Dispatcher *> &active_dispatchers) {

  if (fds_.size() == 0) {
    ::Sleep(timeout);
    return LS_OK_ERROR();
  }
  int num_events = ::WSAPoll(&*fds_.begin(), fds_.size(), timeout);

  if (num_events < 0) {
    if (SOCK_ERRNO() == CERR(EINTR)) {
      return LS_OK_ERROR();
    }
  } else if (num_events == 0) {
    return LS_OK_ERROR();
  } else {
    for (auto it = fds_.begin(); it != fds_.end() && num_events > 0; ++it) {
      if (it->revents > 0) {
        --num_events;
        Dispatcher *dispatcher = dispatchers_[it->fd];
        dispatcher->set_poll_event_data(
            it->revents & (POLLIN | POLLPRI), it->revents & POLLOUT,
            it->revents & POLLERR,
            (it->revents & POLLHUP) && !(it->revents & POLLIN));
        active_dispatchers[dispatcher->get_fd()] = dispatcher;
      }
    }
  }
  return LS_OK_ERROR();
}

std::error_code PollPoller::add_dispatcher(Dispatcher &dispatcher) {
  assert(dispatchers_.find(dispatcher.get_fd()) == dispatchers_.end());
  int sock = dispatcher.get_fd();

  struct pollfd pd;

  pd.fd = sock;
  pd.events = 0;

  if (dispatcher.readable()) {
    pd.events |= POLLIN;
  }
  if (dispatcher.writable()) {
    pd.events |= POLLOUT;
  }
  dispatcher.set_index(fds_.size());
  fds_.push_back(pd);

  dispatchers_[dispatcher.get_fd()] = &dispatcher;
  return LS_OK_ERROR();
}

std::error_code
PollPoller::remove_dispatcher(Dispatcher &dispatcher) {
  assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());

  int sock = dispatcher.get_fd();

  int vec_idx = dispatcher.get_index();
  if (dispatchers_.size() > 1) {
    fds_[vec_idx] = fds_[fds_.size() - 1];
    auto tgt_disp = dispatchers_[fds_[vec_idx].fd];
    tgt_disp->set_index(vec_idx);
    fds_.pop_back();
  } else {
    fds_.clear();
  }

  dispatchers_.erase(dispatcher.get_fd());
  return LS_OK_ERROR();
}

std::error_code
PollPoller::update_dispatcher(Dispatcher &dispatcher) {
  assert(dispatchers_.find(dispatcher.get_fd()) != dispatchers_.end());
  int sock = dispatcher.get_fd();

  struct pollfd &pd = fds_[dispatcher.get_index()];

  pd.events = 0;

  if (dispatcher.readable()) {
    pd.events |= POLLIN;
  }
  if (dispatcher.writable()) {
    pd.events |= POLLOUT;
  }

  return LS_OK_ERROR();
}

} /* network */
} /* light */

#endif
