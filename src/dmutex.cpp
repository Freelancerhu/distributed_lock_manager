#include "dmutex.h"



namespace dlm {
DMutexImpl::DMutexImpl(const std::vector<int> &hosts)
      : hosts_(hosts) {
  }

std::mutex DMutex::mtx_;

DMutex::DMutex(const std::vector<int> &hosts) {
  DMutexImpl temp_dmi(hosts);
  impl_ = std::move(std::make_unique<DMutexImpl>(DMutexImpl(temp_dmi)));
  std::cout << "dmutex constructor" << std::endl;
}

DMutex::~DMutex() {
  std::cout << "~~~~DMutex" << std::endl;
  //db_redis_client_.~DBRedis();
  std::cout << "~~~~~~~~~DMutex" << std::endl;
}

bool DMutex::Lock(const std::string &key, const std::string &value) {
  // try to get lock until get lock
  std::cout << "Lock function starts" << std::endl;
  while (true) {
    std::vector<int> temp_vec = (impl_->hosts_);
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
    for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
      mtx_.lock();
      std::cout << "db #" << temp_vec[lock_index] << std::endl;
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
        mtx_.unlock();
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
      mtx_.lock();
      lock_status_ = true;
      mtx_.unlock();
      //WaitUntil(start_time_milliseconds, run_time_.count());

      timeout_queue_.AsyncRun();
      timeout_queue_.PushEvent([this, key, value]() {
        std::cout << "lambda." << std::endl;
        KeepLock(key, value, lock_validity_time_);
        return true;
      }, std::chrono::milliseconds(500));
      return true;
    }
    else {
      std::cout << "lock failed." << std::endl;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    UnlockAll(key, value);
    WaitUntil(GetCurrentMilliseconds().count(), lock_validity_time_.count());
  }
}




void DMutex::KeepLock(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
  std::cout << "keep lock starts." << std::endl;
  for (int lock_index = 0; lock_index < have_lock_num_.size(); ++lock_index) {
    mtx_.lock();
    if (lock_status_ == false) {
      mtx_.unlock();
      return;
    }
    std::cout << "db #" << lock_index << std::endl;
    DBResult temp_res = db_redis_client_.SelectDB(have_lock_num_[lock_index]);
    if (temp_res == DBResult::kSelectDBSucceed) {
      // try to have a lock
      DBResult temp_set_res = db_redis_client_.UpdateExpire(key, value, lock_validity_time_);
      mtx_.unlock();
      if (temp_set_res == DBResult::kUpdateKeySucceed) {
        std::cout << "Get lock in #" << have_lock_num_[lock_index] << " succeed." << std::endl;
        continue;
      }
      // failed to get the lock, therefore try again.
      else {
        std::cout << "Failed to get the lock in  #" << have_lock_num_[lock_index] << " try again." << std::endl;
        --lock_index;
        continue;
      }
    }
    // select DB failed.
    else if (temp_res == DBResult::kUpdateKeyFailed) {
      std::cout << "Select DB Failed." << std::endl;
      continue;
    }
    else {
      std::cout << "Select DB Function Failed." << std::endl;
      continue;
    }
  }
}





bool DMutex::UnlockAll(const std::string &key, const std::string &value) {
  std::cout << "UnlockAll function starts" << std::endl;
  //timeout_queue_.~TimeoutQueue();
  mtx_.lock();
  lock_status_ = false;
  int temp_lock_size = 0;
  mtx_.unlock();
  for (auto lock_index : (impl_->hosts_)) {
    //select another db
    mtx_.lock();
    DBResult temp_sele_res = db_redis_client_.SelectDB(lock_index);
    if (temp_sele_res == DBResult::kSelectDBSucceed) {
      DBResult temp_del_res = db_redis_client_.DelKeyValue(key, value);
      mtx_.unlock();
      if (temp_del_res == DBResult::kDelKeySucceed) {
        std::cout << "Del key in #" << lock_index << " succeed." << std::endl;
        ++temp_lock_size;
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
  if (temp_lock_size == have_lock_num_.size()) return true;
  return false;
}

bool DMutex::TryLock(const std::string &key, const std::string &value) {
  std::vector<int> temp_vec = (impl_->hosts_);
  uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
  std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
  for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
    mtx_.lock();
    std::cout << "db #" << temp_vec[lock_index] << std::endl;
    //select another db
    DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
    if (temp_res == DBResult::kSelectDBSucceed) {
      // try to have a lock
      DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
      mtx_.unlock();
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

    //WaitUntil(start_time_milliseconds, run_time_.count());
    timeout_queue_.AsyncRun();
    timeout_queue_.PushEvent([this, key, value]() {
      KeepLock(key, value, lock_validity_time_);
      return true;
    }, std::chrono::milliseconds(lock_validity_time_.count() / 3 * 2));
    return true;
  }
  // we did not got enough lock, so we unlock all lock that we have.
  else {
    std::cout << "lock failed." << std::endl;
    UnlockAll(key, value);
    return false;
  }

}

bool DMutex::TryLock(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
  uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
  do {
    std::vector<int> temp_vec = (impl_->hosts_);
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
    for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
      mtx_.lock();
      std::cout << "db #" << temp_vec[lock_index] << std::endl;
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
        mtx_.unlock();
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
      //WaitUntil(start_time_milliseconds, run_time_.count());
      timeout_queue_.AsyncRun();
      timeout_queue_.PushEvent([this, key, value]() {
        KeepLock(key, value, lock_validity_time_);
        return true;
      }, std::chrono::milliseconds(lock_validity_time_.count() / 3 * 2));
      return true;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    else {
      std::cout << "lock failed." << std::endl;
      UnlockAll(key, value);
    }
  } while (GetCurrentMilliseconds().count() - start_time_milliseconds < expire.count());
  return false;
}

std::chrono::milliseconds DMutex::GetCurrentMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch());
}

void DMutex::WaitUntil(const uint16_t &start_time, const uint16_t &limited_time) {
  while (GetCurrentMilliseconds().count() - start_time <= limited_time) {
    //std::cout << "time = " << (GetCurrentMilliseconds().count() - start_time_milliseconds) << std::endl;
  }
}

} // namespace dlm
