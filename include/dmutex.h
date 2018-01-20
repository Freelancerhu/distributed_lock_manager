#ifndef _DLM_DMUTEX_H_
#define _DLM_DMUTEX_H_

#include <string>
#include <vector>
#include <memory>
#include <chrono>>

namespace dlm {
extern enum class DBResult;

class DMutexImpl;

class DMutex {
public:
  DMutex(const std::string &key, std::vector<int> &hosts,
    const std::chrono::milliseconds &temp_lock_validity_time_, const std::chrono::milliseconds &temp_run_time_);
  DMutex(const DMutex &) = delete;
  DMutex(DMutex &&) = default;
  ~DMutex();

  bool lock();
  bool unlock_all();
  bool try_lock();
  bool try_lock(const std::chrono::milliseconds &expire);

private:
  std::unique_ptr<DMutexImpl> impl_;
};

} // namespace DMutex

#endif // _DLM_DMUTEX_H_
