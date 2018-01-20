#include "dmutex.h"
#include "db-redis.h"
#include "timeout-queue.h"

#include <random> // std::default_random_engine
#include <algorithm> // std::shuffle
#include <thread> // std::this_thread::sleep_for
#include <mutex>
#include <memory>

namespace dlm {
  class DMutexImpl {
  public:
    DMutexImpl(const std::string &key, std::vector<int> &hosts,
      const std::chrono::milliseconds &temp_lock_validity_time_, const std::chrono::milliseconds &temp_run_time_);
    DMutexImpl(const DMutexImpl &) = delete;
    DMutexImpl(DMutexImpl &&) = default;
    ~DMutexImpl() = default;

    bool Lock();
    bool UnlockAll();
    bool TryLock();
    bool TryLock(const std::chrono::milliseconds &expire);
    bool MainLockFunction();

  private:
    const std::vector<int> hosts_;
    const std::string key_;
    std::string value_;
    std::chrono::milliseconds lock_validity_time_{ 30000 }; // validity time of lock
    std::chrono::milliseconds run_time_{ 20000 }; // running time which is smaller than lock_validity_time_.
    DBRedis db_redis_client_; // suppose we are accessing Redis just locally from the same computer, and so forth.
                              //only the loopback interface(127.0.0.1)
    std::vector<int> have_lock_num_; // show the number of locks which we have got.
    std::chrono::milliseconds time_out_limit_{ 50 }; // the time prevents the client from remaining blocked for a long time
                                                      //trying to talk with a Redis node which is down
    static std::mutex mtx_;
    //TimeoutQueue timeout_queue_ = TimeoutQueue::TimeoutQueueIns();
    // the status of lock
    bool lock_status_ = false;
    void WaitUntil(std::chrono::milliseconds limited_time);
    void KeepLock();
  };

  std::mutex DMutexImpl::mtx_;

  static std::chrono::milliseconds GetCurrentMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  }

  DMutexImpl::DMutexImpl(const std::string &key, std::vector<int> &hosts,
      const std::chrono::milliseconds &temp_lock_validity_time_, const std::chrono::milliseconds &temp_run_time_)
                                      : key_(key), lock_validity_time_(temp_lock_validity_time_), run_time_(temp_run_time_), hosts_(hosts){
    std::string temp_str = key + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::seed_seq temp_seed(temp_str.begin(), temp_str.end());
    std::mt19937 temp_rand(temp_seed);
    value_ = std::to_string(temp_rand());
  }

  bool DMutexImpl::Lock() {
    // try to get lock until get lock
    while (true) {
      have_lock_num_.clear();
      if (MainLockFunction()) return true;
      WaitUntil(lock_validity_time_ / 3 * 2);
    }
  }

  void DMutexImpl::KeepLock() {
    for (int lock_index = 0; lock_index < have_lock_num_.size(); ++lock_index) {
      mtx_.lock();
      if (lock_status_ == false) {
        mtx_.unlock();
        return;
      }
      DBResult temp_res = db_redis_client_.SelectDB(have_lock_num_[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.UpdateExpire(key_, value_, lock_validity_time_);
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

  bool DMutexImpl::UnlockAll() {
    mtx_.lock();
    lock_status_ = false;
    int temp_lock_size = 0;
    mtx_.unlock();
    for (auto lock_index : hosts_) {
      //select another db
      mtx_.lock();
      DBResult temp_sele_res = db_redis_client_.SelectDB(lock_index);
      if (temp_sele_res == DBResult::kSelectDBSucceed) {
        DBResult temp_del_res = db_redis_client_.DelKeyValue(key_, value_);
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

  bool DMutexImpl::TryLock() {
    return MainLockFunction();
  }

  bool DMutexImpl::TryLock(const std::chrono::milliseconds &expire) {
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    do {
      have_lock_num_.clear();
      if (MainLockFunction()) return true;
      WaitUntil(lock_validity_time_ / 3 * 2);
    } while (GetCurrentMilliseconds().count() - start_time_milliseconds < expire.count());
    return false;
  }

  bool DMutexImpl::MainLockFunction() {
    std::vector<int> temp_vec = hosts_;
    uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
    std::shuffle(temp_vec.begin(), temp_vec.end(), std::default_random_engine(start_time_milliseconds));
    for (int lock_index = 0; lock_index < temp_vec.size(); ++lock_index) {
      mtx_.lock();
      //select another db
      DBResult temp_res = db_redis_client_.SelectDB(temp_vec[lock_index]);
      if (temp_res == DBResult::kSelectDBSucceed) {
        // try to have a lock
        DBResult temp_set_res = db_redis_client_.SetKeyValue(key_, value_, lock_validity_time_);
        mtx_.unlock();
        if (temp_set_res == DBResult::kSetKeySucceed) {
          std::cout << "v = " << value_ << " res = " << temp_vec[lock_index] << std::endl;
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
    if ((have_lock_num_.size() >= (hosts_.size() / 2) + 1) &&
      GetCurrentMilliseconds().count() - start_time_milliseconds <= lock_validity_time_.count()) {
      std::cout << "have = " << have_lock_num_.size() << std::endl;
      std::cout << "host = " << hosts_.size() << std::endl;
      mtx_.lock();
      lock_status_ = true;
      mtx_.unlock();
      TimeoutQueue::TimeoutQueueIns().AsyncRun();
      TimeoutQueue::TimeoutQueueIns().PushEvent([this]() {
        KeepLock();
        return true;
      }, std::chrono::milliseconds(run_time_.count() / 3 * 2));
      return true;
    }
    // we did not got enough lock, so we unlock all lock that we have.
    else {
      UnlockAll();
      return false;
    }
  }

  void DMutexImpl::WaitUntil(std::chrono::milliseconds limited_time) {
    std::this_thread::sleep_for(limited_time);
  }


  DMutex::DMutex(const std::string &key, std::vector<int> &hosts,
    const std::chrono::milliseconds &temp_lock_validity_time_, const std::chrono::milliseconds &temp_run_time_):
  impl_(std::make_unique<DMutexImpl>(key, hosts, temp_lock_validity_time_, temp_run_time_)) {}

  DMutex::~DMutex() {
  }

  bool DMutex::lock() {
    return impl_->Lock();
  }

  bool DMutex::unlock_all() {
    return impl_->UnlockAll();
  }

  bool DMutex::try_lock() {
    return impl_->TryLock();
  }

  bool DMutex::try_lock(const std::chrono::milliseconds &expire) {
    return impl_->TryLock(expire);
  }





} // namespace dlm
