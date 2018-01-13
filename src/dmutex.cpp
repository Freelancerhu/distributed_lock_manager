#include "dmutex.h"



namespace dlm {
DMutexImpl::DMutexImpl(const std::vector<int> &hosts)
      : hosts_(hosts) {
  }

std::atomic_flag DMutex::lock = ATOMIC_FLAG_INIT;

DMutex::DMutex(const std::vector<int> &hosts) {
  DBRedis db_redis_client_("127.0.0.1", 6379);
  DMutexImpl temp_dmi(hosts);
  impl_ = std::move(std::make_unique<DMutexImpl>(DMutexImpl(temp_dmi)));
}

void DMutex::Lock() {
}

void DMutex::Unlock() {
}

bool DMutex::TryLock() {
  return false;
}

void DMutex::Lock(const std::string &key, const std::string &value) {
  // try to get lock until get lock
  std::cout << "Lock function starts" << std::endl;
  while (true) {
    std::vector<int> temp_vec = (impl_->hosts_);
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
    for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
      while (lock.test_and_set(std::memory_order_acquire)) {
        std::cout << "waiting..." << std::endl;
      };
      std::cout << "db #" << temp_vec[lock_index] << std::endl;
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
        lock.clear(std::memory_order_release);
        if (temp_set_res == DBResult::kSetKeySucceed) {
          std::cout << "Get lock in #" << temp_vec[lock_index] << " succeed." << std::endl;
          have_lock_num_.push_back(temp_vec[lock_index]);
          continue;
        }
        // key has been stored.
        else if (temp_set_res == DBResult::kKeyExist) {
          std::cout << "Key exist in #" << temp_vec[lock_index] << std::endl;
          continue;
        }
        // if it is time out, skip current one then try to get next lock.
        else if (GetCurrentMilliseconds().count() - start_time_milliseconds >= time_out_limit_.count()) {
          std::cout << "Time out in #" << temp_vec[lock_index] << ", skip to next DB." << std::endl;
          continue;
        }
        // failed to get the lock, therefore try again.
        else {
          std::cout << "Failed to get the lock in  #" << temp_vec[lock_index] << " try again." << std::endl;
          --lock_index;
          continue;
        }
      }
      // select DB failed.
      else if (temp_res == DBResult::kSelectDBFailed) {
        std::cout << "Select DB Failed." << std::endl;
        continue;
      }
      else {
        std::cout << "Select DB Function Failed." << std::endl;
        continue;
      }
    }
    // If we got enough lock, unlock them until reach lock validity time.
    if ((have_lock_num_.size() >= ((impl_->hosts_).size() / 2) + 1) &&
      (GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count())) {
      std::cout << "lock succeed." << std::endl;
      // std::cout << "time = " << (GetCurrentMilliseconds().count() - start_time_milliseconds) << std::endl;
      while (GetCurrentMilliseconds().count() - start_time_milliseconds <= run_time_.count()) {
         //std::cout << "time = " << (GetCurrentMilliseconds().count() - start_time_milliseconds) << std::endl;
      }
    }
    else {
      std::cout << "lock failed." << std::endl;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    UnlockAll(key, value);
    uint64_t wait_start_time_ = GetCurrentMilliseconds().count();
    while (GetCurrentMilliseconds().count() - wait_start_time_ <= lock_validity_time_.count()) {
      //std::cout << "time = " << (GetCurrentMilliseconds().count() - start_time_milliseconds) << std::endl;
    }
  }
}

void DMutex::UnlockAll(const std::string &key, const std::string &value) {
  std::cout << "UnlockAll function starts" << std::endl;
  for (auto lock_index : (impl_->hosts_)) {
    //select another db
    while (lock.test_and_set(std::memory_order_acquire)) {
      std::cout << "waiting..." << std::endl;
    };
    DBResult temp_sele_res = db_redis_client_.SelectDB(lock_index);
    if (temp_sele_res == DBResult::kSelectDBSucceed) {
      DBResult temp_del_res = db_redis_client_.DelKeyValue(key, value);
      lock.clear(std::memory_order_release);
      if (temp_del_res == DBResult::kDelKeySucceed) {
        std::cout << "Del key in #" << lock_index << " succeed." << std::endl;
        continue;
      }
      else if (temp_del_res == DBResult::kDelKeyFailed) {
        std::cout << "Del key in #" << lock_index << " failed." << std::endl;
        continue;
      }
      else if (temp_del_res == DBResult::kKeyDoesNotExist) {
        std::cout << "Key does not exist in #" << lock_index << std::endl;
        continue;
      }
      else if (temp_del_res == DBResult::kKeyValueWrong) {
        std::cout << "Value of key is wrong in #" << lock_index << std::endl;
      }
      else {
        std::cout << "Del function of #" << lock_index << " failed." << std::endl;
      }
    }
    else if (temp_sele_res == DBResult::kSelectDBFailed) {
      std::cout << "Select DB Failed." << std::endl;
      continue;
    }
    else {
      std::cout << "Select DB Function Failed." << std::endl;
      continue;
    }
  }
}

void DMutex::TryLock(const std::string &key, const std::string &value) {
  std::vector<int> temp_vec = (impl_->hosts_);
  uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
  std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
  for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
    while (lock.test_and_set(std::memory_order_acquire)) {
      std::cout << "waiting..." << std::endl;
    };
    std::cout << "db #" << temp_vec[lock_index] << std::endl;
    //select another db
    DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
    if (temp_res == DBResult::kSelectDBSucceed) {
      // try to have a lock
      DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
      lock.clear(std::memory_order_release);
      if (temp_set_res == DBResult::kSetKeySucceed) {
        std::cout << "Get lock in #" << temp_vec[lock_index] << " succeed." << std::endl;
        have_lock_num_.push_back(temp_vec[lock_index]);
        continue;
      }
      // key has been stored.
      else if (temp_set_res == DBResult::kKeyExist) {
        std::cout << "Key exist in #" << temp_vec[lock_index] << std::endl;
        continue;
      }
      // if it is time out, skip current one then try to get next lock.
      else if (GetCurrentMilliseconds().count() - start_time_milliseconds >= time_out_limit_.count()) {
        std::cout << "Time out in #" << temp_vec[lock_index] << ", skip to next DB." << std::endl;
        continue;
      }
      // failed to get the lock, therefore try again.
      else {
        std::cout << "Failed to get the lock in  #" << temp_vec[lock_index] << " try again." << std::endl;
        --lock_index;
        continue;
      }
    }
    // select DB failed.
    else if (temp_res == DBResult::kSelectDBFailed) {
      std::cout << "Select DB Failed." << std::endl;
      continue;
    }
    else {
      std::cout << "Select DB Function Failed." << std::endl;
      continue;
    }
  }
  // If we got enough lock, unlock them until reach lock validity time.
  if ((have_lock_num_.size() >= ((impl_->hosts_).size() / 2) + 1) &&
    GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
    std::cout << "lock succeed." << std::endl;
    while (GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {}
    UnlockAll(key, value);
  }
  // we did not got enough lock, so we unlock all lock that we have.
  else {
    UnlockAll(key, value);
  }
}

void DMutex::TryLock(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
  uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
  do {
    std::vector<int> temp_vec = (impl_->hosts_);
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
    for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
      while (lock.test_and_set(std::memory_order_acquire)) {
        std::cout << "waiting..." << std::endl;
      };
      std::cout << "db #" << temp_vec[lock_index] << std::endl;
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
        lock.clear(std::memory_order_release);
        if (temp_set_res == DBResult::kSetKeySucceed) {
          std::cout << "Get lock in #" << temp_vec[lock_index] << " succeed." << std::endl;
          have_lock_num_.push_back(temp_vec[lock_index]);
          continue;
        }
        // key has been stored.
        else if (temp_set_res == DBResult::kKeyExist) {
          std::cout << "Key exist in #" << temp_vec[lock_index] << std::endl;
          continue;
        }
        // if it is time out, skip current one then try to get next lock.
        else if (GetCurrentMilliseconds().count() - start_time_milliseconds >= time_out_limit_.count()) {
          std::cout << "Time out in #" << temp_vec[lock_index] << ", skip to next DB." << std::endl;
          continue;
        }
        // failed to get the lock, therefore try again.
        else {
          std::cout << "Failed to get the lock in  #" << temp_vec[lock_index] << " try again." << std::endl;
          --lock_index;
          continue;
        }
      }
      // select DB failed.
      else if (temp_res == DBResult::kSelectDBFailed) {
        std::cout << "Select DB Failed." << std::endl;
        continue;
      }
      else {
        std::cout << "Select DB Function Failed." << std::endl;
        continue;
      }
    }
    // If we got enough lock, unlock them until reach lock validity time.
    if ((have_lock_num_.size() >= ((impl_->hosts_).size() / 2) + 1) &&
      GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
      std::cout << "lock succeed." << std::endl;
      while (GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {}
      UnlockAll(key, value);
      break;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    else {
      UnlockAll(key, value);
    }
  } while (GetCurrentMilliseconds().count() - start_time_milliseconds < expire.count());
}

std::chrono::milliseconds DMutex::GetCurrentMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch());
}



} // namespace dlm
