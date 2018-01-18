#include "db-redis.h"

namespace dlm {
  // DBRedis::DBRedis(){}
  
  DBRedis::DBRedis(const std::string &ip, uint16_t port) {
    //! Enable logging
    client.connect(ip, port,
      [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
      std::cout << "connecting" << std::endl;
      if (status == cpp_redis::client::connect_state::dropped) {
        std::cout << "client disconnected from " << host << ":" << port << std::endl;
      }
    });
  }

  DBRedis::~DBRedis() {
    client.cancel_reconnect();
    client.disconnect(true);
  }

  // sets {key, value} if key does not exist, returns false on failure.
  DBResult DBRedis::SetKeyValue(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
    //std::future< reply > 	set_advanced (const std::string &key, const std::string &value, bool ex=false, int ex_sec=0, 
    //                                bool px=false, int px_milli=0, bool nx=false, bool xx=false)
    //nx(SET if it is not exist)
    auto tempKeyValueFuture = client.set_advanced(key, value, false, 0, true, int(expire.count()), true, false);
    client.sync_commit();
    auto tempRes = tempKeyValueFuture.get();
    if (tempRes.is_null() == true) {
      return DBResult::kKeyExist;
    }
    else if (tempRes.ok() == true) {
      return DBResult::kSetKeySucceed;
    }
    else {
      return DBResult::kSetKeyFailed;
    }
    return DBResult::kSetKeyFunFailed;
  }

  // sets {key, value} if key does not exist or value matches, returns false on failure.
  DBResult DBRedis::UpdateExpire(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
    auto haveKeyFuture = client.exists(std::vector<std::string>{key});
    client.sync_commit();
    // show if the key exists
    uint64_t tempExistFlag = haveKeyFuture.get().as_integer();
    if (tempExistFlag == 0) {
      // the key does not exist, set {key, value, pexpire}
      return SetKeyValue(key, value, expire);
    }
    else {
      // the key exists, show if the value is right
      auto haveValueFuture = client.get(key);
      client.sync_commit();
      std::string tempValue = haveValueFuture.get().as_string();
      if (tempValue == value) {
        // the value is right, update the pexpire
        auto setPexpire = client.pexpire(key, int(expire.count()));
        client.sync_commit();
        uint64_t tempSetPexpireFlag = setPexpire.get().as_integer();
        if (tempSetPexpireFlag == 0) {
          return DBResult::kUpdateKeyFailed;
        }
        else if (tempSetPexpireFlag == 1) {
          return DBResult::kUpdateKeySucceed;
        }
      }
      else {
        // the value is wrong, return false
        return DBResult::kKeyValueWrong;
      }
    }
    return DBResult::kUpdateKeyFunFailed;
  }

  // deletes {key, value} if value matches, returns false on failure.
  DBResult DBRedis::DelKeyValue(const std::string &key, const std::string &value) {
    auto haveKeyFuture = client.exists(std::vector<std::string>{key});
    client.sync_commit();
    // show if the key exists
    uint64_t tempExistFlag = haveKeyFuture.get().as_integer();
    if (tempExistFlag == 0) {
      // the key does not exist, set {key, value, pexpire}
      return DBResult::kKeyDoesNotExist;
    }
    else {
      // the key exists, show if the value is right
      auto haveValueFuture = client.get(key);
      client.sync_commit();
      std::string tempValue = haveValueFuture.get().as_string();
      if (tempValue == value) {
        // the value is right, update the pexpire
        auto delKey = client.del(std::vector<std::string>{key});
        client.sync_commit();
        uint64_t tempDelFlag = delKey.get().as_integer();
        if (tempDelFlag == 0) {
          return DBResult::kDelKeyFailed;
        }
        else if (tempDelFlag == 1) {
          return DBResult::kDelKeySucceed;
        }
      }
      else {
        // the value is wrong, return false
        return DBResult::kKeyValueWrong;
      }
    }
    return DBResult::kDelKeyFunFailed;
  }

  DBResult DBRedis::SelectDB(const int &index) {
    auto selectFlag = client.select(index);
    client.sync_commit();
    std::string tempSelectFlag = selectFlag.get().as_string();
    if (tempSelectFlag == "OK")
      return DBResult::kSelectDBSucceed;
    else if (tempSelectFlag == "ERR invalid DB index")
      return DBResult::kSelectDBFailed;
    return DBResult::kSelectFunFailed;
  }
} // namespace dlm

