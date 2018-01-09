#include "DbRedisInterface.h"

namespace dlm {
  DbRedisInterface::DbRedisInterface(const std::string &ip, uint16_t port) {
    //! Enable logging
    client.connect(ip, port,
      [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
      if (status == cpp_redis::client::connect_state::dropped) {
        std::cout << "client disconnected from " << host << ":" << port << std::endl;
      }
    });
  }

  // sets {key, value} if key does not exist, returns false on failure.
  bool DbRedisInterface::SetKeyValue(const std::string &key, std::string value, std::chrono::milliseconds expire) {
    //std::future< reply > 	set_advanced (const std::string &key, const std::string &value, bool ex=false, int ex_sec=0, 
    //                                bool px=false, int px_milli=0, bool nx=false, bool xx=false)
    //nx(SET if it is not exist)
    std::cout << "SetKeyValue starts." << std::endl;
    auto tempKeyValueFuture = client.set_advanced(key, value, false, 0,
      true, int(expire.count()), true, false);
    client.sync_commit();
    auto tempRes = tempKeyValueFuture.get();
    if (tempRes.is_null() == true) {
      std::cout << "This key has been set" << std::endl;
      return false;
    }
    else if (tempRes.ok() == true) {
      std::cout << "SetKeyValue succeed." << std::endl;
      return true;
    }
    else {
      std::cout << "SetKeyValue failed." << std::endl;
      return false;
    }
    std::cout << "Oops, in SetKeyValue, some creepy things happened." << std::endl;
    return false;
  }

  // sets {key, value} if key does not exist or value matches, returns false on failure.
  bool DbRedisInterface::UpdateExpire(const std::string &key, std::string value, std::chrono::milliseconds expire) {
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
          return false;
        }
        else if (tempSetPexpireFlag == 1) {
          std::cout << "UpdateExpire succceed." << std::endl;
          return true;
        }
      }
      else {
        // the value is wrong, return false
        std::cout << "The value is wrong." << std::endl;
        return false;
      }
    }
    std::cout << "Oops, in UpdateExpire, some creepy things happened." << std::endl;
    return false;
  }

  // deletes {key, value} if value matches, returns false on failure.
  bool DbRedisInterface::DelKeyValue(const std::string &key, std::string value) {
    auto haveKeyFuture = client.exists(std::vector<std::string>{key});
    client.sync_commit();
    // show if the key exists
    uint64_t tempExistFlag = haveKeyFuture.get().as_integer();
    if (tempExistFlag == 0) {
      // the key does not exist, set {key, value, pexpire}
      std::cout << "The key does not exist." << std::endl;
      return false;
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
          return false;
        }
        else if (tempDelFlag == 1) {
          std::cout << "DelKey succceed." << std::endl;
          return true;
        }
      }
      else {
        // the value is wrong, return false
        std::cout << "The value is wrong." << std::endl;
        return false;
      }
    }
    std::cout << "Oops, in DelKeyValue, some creepy things happened." << std::endl;
    return false;
  }

  void DbRedisInterface::DBInterfaceTest() {
    std::chrono::milliseconds e{ 1000000 };
    std::chrono::milliseconds r{ 5000000 };
    std::cout << "interface test." << std::endl;
    std::cout << "=======setkeyvalue=======" << std::endl;
    std::cout << "the result is " << SetKeyValue("hu", "1223", e) << std::endl;
    std::cout << "=======updateexpire=======" << std::endl;
    std::cout << "the result is " << UpdateExpire("hu", "11223", r) << std::endl;
    std::cout << "=======delkeyvalue=======" << std::endl;
    std::cout << "the result is " << DelKeyValue("hu", "1223") << std::endl;


  }


} // namespace dlm

