#ifndef _DLM_DMUTEX_H_
#define _DLM_DMUTEX_H_

#include <memory>
#include <string>
#include <vector>

namespace dlm {
struct DMutexImpl;

class DMutex {
public:
  DMutex(const std::vector<std::string> &hosts);
  DMutex(const DMutex &) = delete;
  DMutex(DMutex &&) = default;

  void lock();
  void unlock();
  bool try_lock();

private:
  std::unique_ptr<DMutexImpl> impl_;
};

} // namespace DMutex

#endif // _DLM_DMUTEX_H_
