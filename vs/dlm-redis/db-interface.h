#ifndef _DLM_DB_INTERFACE_H_
#define _DLM_DB_INTERFACE_H_

#include "cpp_redis/cpp_redis"

#include <chrono>
#include <string>
#include <vector>

namespace dlm {
class DBInterface {
public:
  DBInterface(const std::string &ip = "127.0.0.1", uint16_t port = 6379);

  // sets {key, value} if key does not exist, returns false on failure.
  virtual bool SetKeyValue(const std::string &key, uint64_t value, std::chrono::milliseconds expire);

  // sets {key, value} if key does not exist or value matches, returns false on failure.
  virtual bool UpdateExpire(const std::string &key, uint64_t value, std::chrono::milliseconds expire);

  // deletes {key, value} if value matches, returns false on failure.
  virtual bool DelKeyValue(const std::string &key, uint64_t value);

  virtual ~DBInterface() = default;
  
  void DBInterfaceTest();

private:
  cpp_redis::client client;
  
};

} // namespace dlm

#endif // _DLM_DB_INTERFACE_H_
