#ifndef _DLM_DB_INTERFACE_H_
#define _DLM_DB_INTERFACE_H_

#include "cpp_redis/cpp_redis"

#include <chrono>
#include <string>
#include <vector>

namespace dlm {
class DBInterface {
public:
  // sets {key, value} if key does not exist, returns false on failure.
  virtual bool SetKeyValue(const std::string &key, std::string value, std::chrono::milliseconds expire) = 0;

  // sets {key, value} if key does not exist or value matches, returns false on failure.
  virtual bool UpdateExpire(const std::string &key, std::string value, std::chrono::milliseconds expire) = 0;

  // deletes {key, value} if value matches, returns false on failure.
  virtual bool DelKeyValue(const std::string &key, std::string value) = 0;

  virtual ~DBInterface() = default;
};

} // namespace dlm

#endif // _DLM_DB_INTERFACE_H_
