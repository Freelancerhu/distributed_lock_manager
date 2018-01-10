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
    : impl_(std::make_unique<DMutexImpl>(hosts)) {
  DBRedis* tempDBRedis = new DBRedis("127.0.0.1", 6379);
  db_redis_client_.push_back(tempDBRedis);
}

void DMutex::Lock() {
  // (impl_->hosts_)[0]
}

void DMutex::Unlock() {
}

bool DMutex::TryLock() {
  return false;
}

void DMutex::Lock(const std::string &key, const std::string &value) {
  uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
  // std::cout << start_time_milliseconds << std::endl; 
  for (int lock_index = 0; lock_index < db_redis_client_.size(); ++lock_index) {
    // try to have a lock
    if (db_redis_client_[lock_index]->SetKeyValue(key, value, lock_validity_time_) == DBResult::kSetKeySucceed) {
      have_lock_num_.push_back(lock_index);
      continue;
    }
    // if it is time out, skip current one then try to get next lock.
    else if (GetCurrentMilliseconds().count() - start_time_milliseconds >= time_out_limit_.count()) {
      continue;
    }
    // failed to get the lock, therefore try again.
    else {
      --lock_index;
      continue;
    }
  }
  if ((have_lock_num_.size() >= (db_redis_client_.size() / 2) + 1) && 
    GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
    std::cout << "lock succeed." << std::endl;
    while (GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
    }
    UnlockAll(key, value);
  }
  else {
    UnlockAll(key, value);
  }

}

void DMutex::UnlockAll(const std::string &key, const std::string &value) {
  for (auto lock_index : have_lock_num_) {
    if (db_redis_client_[lock_index]->DelKeyValue(key, value) == DBResult::kDelKeySucceed) {
      continue;
    }
    else {
      std::cout << "failed to unlock lock." << std::endl;
    }
  }
}

std::chrono::milliseconds DMutex::GetCurrentMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch());
}



} // namespace dlm
