#include "timeout-queue.h"




void TimeoutQueue::Worker() {
  std::cout << "timeout-queue worker starts." << std::endl;
  while (!quit_.load()) { // not quit
    UniqueLockType<MutexType> lock(mtx_);
    if (time_out_queue_.empty()) {
      if (events_out_of_queue_.load() == 0)
        all_done_.notify_all();
      new_event_.wait(lock, [this] { // if it is not quit or time_out_queue_ is empty, wait for new_event_
            return quit_.load() || !time_out_queue_.empty();
          });
    }

    if (quit_.load())
      break;
    
    ++events_out_of_queue_;
    auto node = time_out_queue_.extract(time_out_queue_.begin());
    // std::multimap<TimePoint, Event>;
    auto [next_timeout, next_event] = std::tie(node.key(), node.mapped());
    
    new_event_.wait_until(lock, next_timeout, [this, &next_timeout] {
        return quit_.load() ||
            !time_out_queue_.empty() && next_timeout > time_out_queue_.begin()->first;
      });
    if (!quit_.load() && next_timeout <= Clock::now()) { // Clock::now() reach the next_timeout
      lock.unlock();
      const auto is_repeat = next_event.function();
      if (is_repeat) { // the pre funtion is true, set the next_timeout then insert the node into the time_out_queue_
        next_timeout += next_event.period;

        lock.lock();
        time_out_queue_.insert(std::move(node));
        new_event_.notify_one();
      }
    } else { // does not reach the next_timeout, insert the node into the time_out_queue_
      time_out_queue_.insert(std::move(node));
    }
  }
}
