#include "dmutex.h"



namespace dlm {
DMutexImpl::DMutexImpl(const std::vector<int> &hosts)
      : hosts_(hosts) {
  }

std::mutex DMutex::mtx_;

DMutex::DMutex(const std::vector<int> &hosts, const std::chrono::milliseconds &temp_lock_validity_time_, const std::chrono::milliseconds &temp_run_time_) 
                                    : lock_validity_time_(temp_lock_validity_time_), run_time_(temp_run_time_){
  DMutexImpl temp_dmi(hosts);
  impl_ = std::move(std::make_unique<DMutexImpl>(DMutexImpl(temp_dmi)));
}

DMutex::~DMutex() {
}

bool DMutex::Lock(const std::string &key, const std::string &value) {
  // try to get lock until get lock
  while (true) {
    std::vector<int> temp_vec = (impl_->hosts_);
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
    for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
      mtx_.lock();
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
        mtx_.unlock();
        if (temp_set_res == DBResult::kSetKeySucceed) {
          have_lock_num_.push_back(temp_vec[lock_index]);
          continue;
        }
        // key has been stored.
        else if (temp_set_res == DBResult::kKeyExist) {
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
    }
    // If we got enough lock, unlock them until reach lock validity time.
    if ((have_lock_num_.size() >= ((impl_->hosts_).size() / 2) + 1) &&
      (GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count())) {
      mtx_.lock();
      lock_status_ = true;
      mtx_.unlock();
      //WaitUntil(start_time_milliseconds, run_time_.count());
      timeout_queue_.AsyncRun();
      timeout_queue_.PushEvent([this, key, value]() {
        KeepLock(key, value, lock_validity_time_);
        return true;
      }, std::chrono::milliseconds(run_time_.count() /3 * 2));
      return true;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    UnlockAll(key, value);
    WaitUntil(GetCurrentMilliseconds().count(), lock_validity_time_.count());
  }
}




void DMutex::KeepLock(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
  for (int lock_index = 0; lock_index < have_lock_num_.size(); ++lock_index) {
    mtx_.lock();
    if (lock_status_ == false) {
      mtx_.unlock();
      return;
    }
    DBResult temp_res = db_redis_client_.SelectDB(have_lock_num_[lock_index]);
    if (temp_res == DBResult::kSelectDBSucceed) {
      // try to have a lock
      DBResult temp_set_res = db_redis_client_.UpdateExpire(key, value, lock_validity_time_);
      mtx_.unlock();
      if (temp_set_res == DBResult::kUpdateKeySucceed) {
        continue;
      }
      // failed to get the lock, therefore try again.
      else {
        --lock_index;
        continue;
      }
    }
  }
}





bool DMutex::UnlockAll(const std::string &key, const std::string &value) {
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
        ++temp_lock_size;
        continue;
      }
      else if (temp_del_res == DBResult::kDelKeyFailed || temp_del_res == DBResult::kKeyDoesNotExist || temp_del_res == DBResult::kKeyValueWrong) {
        continue;
      }
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
    //select another db
    DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
    if (temp_res == DBResult::kSelectDBSucceed) {
      // try to have a lock
      DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
      mtx_.unlock();
      if (temp_set_res == DBResult::kSetKeySucceed) {
        have_lock_num_.push_back(temp_vec[lock_index]);
        continue;
      }
      // key has been stored.
      else if (temp_set_res == DBResult::kKeyExist || GetCurrentMilliseconds().count() - start_time_milliseconds >= time_out_limit_.count()) {
        continue;
      }
      // failed to get the lock, therefore try again.
      else {
        --lock_index;
        continue;
      }
    }
  }
  // If we got enough lock, unlock them until reach lock validity time.
  if ((have_lock_num_.size() >= ((impl_->hosts_).size() / 2) + 1) &&
    GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
    mtx_.lock();
    lock_status_ = true;
    mtx_.unlock();
    //WaitUntil(start_time_milliseconds, run_time_.count());
    timeout_queue_.AsyncRun();
    timeout_queue_.PushEvent([this, key, value]() {
      KeepLock(key, value, lock_validity_time_);
      return true;
    }, std::chrono::milliseconds(run_time_.count() / 3 * 2));
    return true;
  }
  // we did not got enough lock, so we unlock all lock that we have.
  else {
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
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key, value, lock_validity_time_);
        mtx_.unlock();
        if (temp_set_res == DBResult::kSetKeySucceed) {
          have_lock_num_.push_back(temp_vec[lock_index]);
          continue;
        }
        // key has been stored.
        else if (temp_set_res == DBResult::kKeyExist || GetCurrentMilliseconds().count() - start_time_milliseconds >= time_out_limit_.count()) {
          continue;
        }
        // failed to get the lock, therefore try again.
        else {
          --lock_index;
          continue;
        }
      }
    }
    // If we got enough lock, unlock them until reach lock validity time.
    if ((have_lock_num_.size() >= ((impl_->hosts_).size() / 2) + 1) &&
      GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
      mtx_.lock();
      lock_status_ = true;
      mtx_.unlock();
      //WaitUntil(start_time_milliseconds, run_time_.count());
      timeout_queue_.AsyncRun();
      timeout_queue_.PushEvent([this, key, value]() {
        KeepLock(key, value, lock_validity_time_);
        return true;
      }, std::chrono::milliseconds(run_time_.count() / 3 * 2));
      return true;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    else {
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
  }
}

} // namespace dlm
