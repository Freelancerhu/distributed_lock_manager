#include "dmutex.h"

namespace dlm {
struct DMutexImpl {
public:
  friend class DMutex;

  DMutexImpl(const std::vector<std::string> &hosts)
      : hosts_(hosts) {}

private:
  const std::vector<std::string> hosts_;
};

DMutex::DMutex(const std::vector<std::string> &hosts)
    : impl_(std::make_unique<DMutexImpl>(hosts)) {}

void DMutex::lock() {
}

void DMutex::unlock() {
}

bool DMutex::try_lock() {
  return false;
}

} // namespace dlm
