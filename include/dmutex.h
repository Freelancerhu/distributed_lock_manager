#ifndef _DLM_DMUTEX_H_
#define _DLM_DMUTEX_H_

#include "db-redis.h"

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <random> // std::default_random_engine
#include <algorithm> // std::shuffle
#include <thread> // std::this_thread::sleep_for
#include <atomic> // atomic_flag

namespace dlm {
extern enum class DBResult;

struct DMutexImpl {
public:
  friend class DMutex;

  DMutexImpl(const std::vector<int> &hosts);

private:
  const std::vector<int> hosts_;
};

class DMutex {
public:
  DMutex(const std::vector<int> &hosts);
  DMutex(const DMutex &) = delete;
  DMutex(DMutex &&) = default;

  void Lock();
  void Unlock();
  bool TryLock();

  void Lock(const std::string &key, const std::string &value);
  void UnlockAll(const std::string &key, const std::string &value);
  void TryLock(const std::string &key, const std::string &value);
  void TryLock(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire);
  std::chrono::milliseconds GetCurrentMilliseconds();

private:
  std::unique_ptr<DMutexImpl> impl_;
  std::chrono::milliseconds lock_validity_time_{ 60000 };
  std::chrono::milliseconds run_time_{ 40000 }; // running time which is smaller than lock_validity_time_.
  DBRedis db_redis_client_; // suppose we are accessing Redis just locally from the same computer, and so forth.
                                                      //only the loopback interface(127.0.0.1)
  std::vector<int> have_lock_num_; // show the number of locks which we have got.
  std::chrono::milliseconds time_out_limit_{ 50 }; // the time prevents the client from remaining blocked for a long time
                                             //trying to talk with a Redis node which is down
  static std::atomic_flag lock;
};

} // namespace DMutex

#endif // _DLM_DMUTEX_H_
