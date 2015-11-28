#include "network/looper.h"

namespace light {
namespace network {
Looper::Looper()
    : poller_(Poller::create_default_poller(*this)), stop_(false),
      valid_dispatchers_(), queue_(), loop_callbacks_(), last_callback_idx_(0) {
  running_workers_.store(0);
  int eventfd = 0, timerfd = 0;
  bool commit = false;
  SCOPE_EXIT([&eventfd, &timerfd, &commit] {
    if (!commit) {
#ifdef HAVE_TIMERFD
      if (eventfd > 0)
        ::close(eventfd);
#endif
#ifdef HAVE_EVENTFD
      if (timerfd > 0)
        ::close(timerfd);
#endif
    }
  });

  light::utils::ErrorCode ec;

#ifdef HAVE_TIMERFD
  // many cause leap second problem?
  if ((timerfd = ::timerfd_create(CLOCK_REALTIME,
                                  TFD_NONBLOCK | TFD_CLOEXEC)) == -1) {
    throw light::exception::EventException(LS_GENERIC_ERROR(errno));
  }
  timer_dispatcher_.reset(new Dispatcher(*this, timerfd));
  ec = timer_dispatcher_->attach();
  if (!ec.ok())
    throw light::exception::EventException(ec);
  // set timer callback
  timer_dispatcher_->enable_read();
  timer_dispatcher_->set_read_callback([timerfd, this] {
    uint64_t times;
    ::read(timerfd, &times, sizeof times);
    // check timers and call callbacks
    queue_.update_time_now();
    // update nearist timer
    update_timerfd_expire();
  });
#endif

#ifdef HAVE_EVENTFD
  if ((eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) == -1) {
    throw light::exception::EventException(LS_GENERIC_ERROR(errno));
  }
  event_dispatcher_.reset(new Dispatcher(*this, eventfd));
#elif defined(HAVE_UNISTD_H)
  int pipes[2];
  if (::pipe(pipes) == -1) {
    throw light::exception::EventException(LS_GENERIC_ERROR(errno));
  }
  if (light::utils::set_nonblocking(pipes[0]) != 0) {
    throw light::exception::EventException(LS_GENERIC_ERROR(errno));
  }
  if (light::utils::set_nonblocking(pipes[1]) != 0) {
    throw light::exception::EventException(LS_GENERIC_ERROR(errno));
  }
  event_dispatcher_.reset(new Dispatcher(*this, pipes[0]));
  w_event_dispatcher_.reset(new Dispatcher(*this, pipes[1]));
  eventfd = pipes[0];

  ec = event_dispatcher_->attach();
  if (!ec.ok())
    throw light::exception::EventException(ec);

  event_dispatcher_->enable_read();

  event_dispatcher_->set_read_callback([eventfd, this] {
    char buf[128];
    ::recv(eventfd, buf, 128, 0);
  });

  this->add_timer(ec, 1000, 1000, [this] {
    for (auto &kv : loop_callbacks_) {
      kv.second();
    }
  });
#endif
  commit = true;
}

light::utils::ErrorCode Looper::add_dispatcher(Dispatcher &dispatcher) {
  return poller_->add_dispatcher(dispatcher);
}

light::utils::ErrorCode Looper::remove_dispatcher(Dispatcher &dispatcher) {
  return poller_->remove_dispatcher(dispatcher);
}

light::utils::ErrorCode Looper::update_dispatcher(Dispatcher &dispatcher) {
  return poller_->update_dispatcher(dispatcher);
}

Dispatcher &Looper::get_time_dispatcher() { return *timer_dispatcher_; }

Dispatcher &Looper::get_event_dispatcher() { return *event_dispatcher_; }

TimerId Looper::add_timer(light::utils::ErrorCode &ec, Timestamp timeout,
                          Timestamp interval, const functor &time_callback) {
  ec = LS_OK_ERROR();
  bool need_update;
  auto ret = queue_.add_timer(timeout, interval, need_update, time_callback);
  if (need_update) {
    ec = update_timerfd_expire();
  }
  return ret;
}

void Looper::cancel_timer(light::utils::ErrorCode &ec, const TimerId timer_id) {
  ec = LS_OK_ERROR();
  bool need_update;
  queue_.del_timer(timer_id, need_update);
  if (need_update) {
    ec = update_timerfd_expire();
  }
}

light::utils::ErrorCode Looper::update_timerfd_expire() {
#ifdef HAVE_TIMERFD
  auto timer = queue_.get_nearist_timer();
  struct itimerspec spec;
  spec.it_interval.tv_sec = 0;
  spec.it_interval.tv_nsec = 0;
  if (timer) {
    auto next = timer->get_next();
    spec.it_value.tv_sec = next / 1000000LL;
    spec.it_value.tv_nsec = next % 1000000LL * 1000;
  } else {
    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 0;
  }
  if ((::timerfd_settime(timer_dispatcher_->get_fd(), TFD_TIMER_ABSTIME, &spec,
                         nullptr)) == -1) {
    return LS_GENERIC_ERROR(errno);
  }
#endif
  return LS_OK_ERROR();
}

void Looper::stop() { stop_ = true; }

void Looper::loop() {
  while (!stop_) {
    bool should_poll = false;
    int nowval = running_workers_.load();
    if (nowval) {
      should_poll = false;
    } else {
      if (running_workers_.compare_exchange_weak(nowval, nowval + 1)) {
        should_poll = true;
      } else {
        should_poll = false;
      }
    }

    if (should_poll) {
      {
        std::unique_lock<std::mutex> lk(cond_lock_);
        notify_valid_ = 0;
      }

      valid_dispatchers_.clear();
#ifdef HAVE_TIMERFD
      const int64_t tick_milisec = -1;
#else
      const int64_t tick_milisec = 1;
#endif

      double load = 0;
      Timestamp start = light::utils::get_timestamp();
      auto ec = poller_->poll(tick_milisec, valid_dispatchers_);
      Timestamp interval = light::utils::get_timestamp() - start;

#ifndef HAVE_TIMERFD
      tick_timer();
#endif
      for (auto &kv : valid_dispatchers_) {
        Dispatcher *disp = kv.second;
        post_functors_.push_back([disp]() { disp->handle_events(); });
      }

      if (!ec.ok()) {
        throw light::exception::EventException(ec);
      }
      // waik up other threads
      {
        std::unique_lock<std::mutex> lk(cond_lock_);
        notify_valid_ = 1;
      }
      cond_var_.notify_all();
      running_workers_.fetch_sub(1);
      functors_work();
    } else {
      // this is work thread
      std::unique_lock<std::mutex> lk(cond_lock_);
      // wait for signal
      cond_var_.wait(lk, [this] { return notify_valid_ == 1; });
      // wake up!
      functors_work();
    }
  }
  // std::notify_all_at_thread_exit
}

void Looper::functors_work() {
  running_workers_.fetch_add(1);
  while (true) {
    std::function<void()> func;
    do {
      std::unique_lock<std::mutex> plk(post_functor_lock_);
      // handle unsafe post functors
      if (!post_functors_.empty()) {
        int slice_count = (std::max)(post_functors_.size() /
                                         std::thread::hardware_concurrency(),
                                     static_cast<size_t>(1));
        std::vector<functor> vec;
        for (int i = 0; i < slice_count; ++i) {
          if (post_functors_.empty())
            break;
          vec.emplace_back(std::move(post_functors_.front()));
          post_functors_.pop_front();
        }
        func = std::bind([](const std::vector<functor> &func_vec) {
          for (auto &v : func_vec) {
            v();
          }
        }, std::move(vec));
        break;
      }

      // handle safe post functors
      if (!unique_post_functors_.empty()) {
        for (auto it = unique_post_functors_.begin();
             it != unique_post_functors_.end(); ++it) {
          auto unique_id = it->first;
          if (running_post_queue_ids_.find(unique_id) !=
              running_post_queue_ids_.end())
            continue;
          running_post_queue_ids_.insert(unique_id);
          func = std::bind([this, unique_id](const std::vector<functor> &vec) {
            for (auto &v : vec) {
              v();
            }
            std::unique_lock<std::mutex> pfk(post_functor_lock_);
            running_post_queue_ids_.erase(unique_id);
          }, std::move(it->second));
          unique_post_functors_.erase(it);
          break;
        }
      }
    } while (false);

    if (func) {
      func();
    } else {
      break;
    }
  }
  running_workers_.fetch_sub(1);
}

int Looper::register_loop_callback(const loop_callback_t &func, int idx) {
  std::unique_lock<std::mutex> plk(post_functor_lock_);
  if (idx == -1)
    idx = ++last_callback_idx_;
  assert(loop_callbacks_.find(idx) == loop_callbacks_.end());
  loop_callbacks_[idx] = func;
  return idx;
}

void Looper::unregister_loop_callback(int idx) {
  std::unique_lock<std::mutex> plk(post_functor_lock_);
  assert(loop_callbacks_.find(idx) != loop_callbacks_.end());
  loop_callbacks_.erase(idx);
}

void Looper::tick_timer() {
  queue_.update_time_now();
  update_timerfd_expire();
}

} /* network */
} /* light */
