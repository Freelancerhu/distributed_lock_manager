#ifndef _DLM_DMUTEX_H_
#define _DLM_DMUTEX_H_

#include "db-redis.h"

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>

namespace dlm {
extern enum class DBResult;

struct DMutexImpl;

class DMutex {
public:
  DMutex(const std::vector<std::string> &hosts);
  DMutex(const DMutex &) = delete;
  DMutex(DMutex &&) = default;

  void Lock();
  void Unlock();
  bool TryLock();

  void Lock(const std::string &key, const std::string &value);
  void UnlockAll(const std::string &key, const std::string &value);
  std::chrono::milliseconds GetCurrentMilliseconds();

private:
  std::unique_ptr<DMutexImpl> impl_;
  std::chrono::milliseconds lock_validity_time_{ 1000000 };
  std::chrono::milliseconds run_time_; // running time which is smaller than lock_validity_time_.
  std::vector<DBRedis*> db_redis_client_; // suppose we are accessing Redis just locally from the same computer, and so forth.
                                                      //only the loopback interface(127.0.0.1)
  std::vector<int> have_lock_num_; // show the number of locks which we have got.
  std::chrono::milliseconds time_out_limit_{ 50 }; // the time prevents the client from remaining blocked for a long time
                                             //trying to talk with a Redis node which is down
};

} // namespace DMutex

#endif // _DLM_DMUTEX_H_
