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

  // sets {key, value} if key does not exist, returns false on failure.
  DBResult DBRedis::SetKeyValue(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire) {
    //std::future< reply > 	set_advanced (const std::string &key, const std::string &value, bool ex=false, int ex_sec=0, 
    //                                bool px=false, int px_milli=0, bool nx=false, bool xx=false)
    //nx(SET if it is not exist)
    std::cout << "SetKeyValue starts." << std::endl;
    auto tempKeyValueFuture = client.set_advanced(key, value, false, 0, true, int(expire.count()), true, false);
    client.sync_commit();
    auto tempRes = tempKeyValueFuture.get();
    if (tempRes.is_null() == true) {
      std::cout << "This key has been set" << std::endl;
      return DBResult::kKeyExist;
    }
    else if (tempRes.ok() == true) {
      std::cout << "SetKeyValue succeed." << std::endl;
      return DBResult::kSetKeySucceed;
    }
    else {
      std::cout << "SetKeyValue failed." << std::endl;
      return DBResult::kSetKeyFailed;
    }
    std::cout << "Oops, in SetKeyValue, some creepy things happened." << std::endl;
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
      std::cout << "The key does not exist." << std::endl;
      return SetKeyValue(key, value, expire);
    }
    else {
      // the key exists, show if the value is right
      std::cout << "The key exists." << std::endl;
      auto haveValueFuture = client.get(key);
      client.sync_commit();
      std::string tempValue = haveValueFuture.get().as_string();
      if (tempValue == value) {
        // the value is right, update the pexpire
        std::cout << "The value is right." << std::endl;
        auto setPexpire = client.pexpire(key, int(expire.count()));
        client.sync_commit();
        uint64_t tempSetPexpireFlag = setPexpire.get().as_integer();
        if (tempSetPexpireFlag == 0) {
          std::cout << "UpdateExpire failed." << std::endl;
          return DBResult::kUpdateKeyFailed;
        }
        else if (tempSetPexpireFlag == 1) {
          std::cout << "UpdateExpire succceed." << std::endl;
          return DBResult::kUpdateKeySucceed;
        }
      }
      else {
        // the value is wrong, return false
        std::cout << "The value is wrong." << std::endl;
        return DBResult::kKeyValueWrong;
      }
    }
    std::cout << "Oops, in UpdateExpire, some creepy things happened." << std::endl;
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
      std::cout << "The key does not exist." << std::endl;
      return DBResult::kKeyDoesNotExist;
    }
    else {
      // the key exists, show if the value is right
      std::cout << "The key exists." << std::endl;
      auto haveValueFuture = client.get(key);
      client.sync_commit();
      std::string tempValue = haveValueFuture.get().as_string();
      if (tempValue == value) {
        // the value is right, update the pexpire
        std::cout << "The value is right." << std::endl;
        auto delKey = client.del(std::vector<std::string>{key});
        client.sync_commit();
        uint64_t tempDelFlag = delKey.get().as_integer();
        if (tempDelFlag == 0) {
          std::cout << "DelKey failed." << std::endl;
          return DBResult::kDelKeyFailed;
        }
        else if (tempDelFlag == 1) {
          std::cout << "DelKey succceed." << std::endl;
          return DBResult::kDelKeySucceed;
        }
      }
      else {
        // the value is wrong, return false
        std::cout << "The value is wrong." << std::endl;
        return DBResult::kKeyValueWrong;
      }
    }
    std::cout << "Oops, in DelKeyValue, some creepy things happened." << std::endl;
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

  void DBRedis::DBInterfaceTest() {
    //auto selectFlag = client.select(1);
    //client.sync_commit();
    //std::string tempDelFlag = selectFlag.get().as_string();
    //if (tempDelFlag == "OK")
    //  std::cout << "Success string." << std::endl;
    //std::cout << "select = " << tempDelFlag << std::endl;
    //selectFlag = client.select(100000);
    //client.sync_commit();
    //tempDelFlag = selectFlag.get().as_string();
    //if (tempDelFlag == "ERR invalid DB index")
    //  std::cout << "fail string." << std::endl;
    //std::cout << "select = " << tempDelFlag << std::endl;

    std::chrono::milliseconds e{ 1000000 };
    std::chrono::milliseconds r{ 5000000 };
    std::cout << "interface test." << std::endl;
    std::cout << "=======setkeyvalue=======" << std::endl;
    std::cout << "the result is " << static_cast<std::underlying_type<DBResult>::type>(SetKeyValue("hu", "1223", e)) << std::endl;
    std::cout << "the result is " << static_cast<std::underlying_type<DBResult>::type>(SetKeyValue("hu", "1333", e)) << std::endl;
    //std::cout << "=======updateexpire=======" << std::endl;
    //std::cout << "the result is " << static_cast<std::underlying_type<DBResult>::type>(UpdateExpire("hu", "1223", r)) << std::endl;
    //std::cout << "=======delkeyvalue=======" << std::endl;
    //std::cout << "the result is " << static_cast<std::underlying_type<DBResult>::type>(DelKeyValue("hu", "12123")) << std::endl;

    std::cout << "function finished." << std::endl;
  }


} // namespace dlm

