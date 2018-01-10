#ifndef _DLM_DB_INTERFACE_H_
#define _DLM_DB_INTERFACE_H_

#include "cpp_redis/cpp_redis"

#include <chrono>
#include <string>
#include <vector>

namespace dlm {
enum class DBResult {
  kKeyExist,
  kSetKeySucceed,
  kSetKeyFailed,
  kSetKeyFunFailed,
  kKeyDoesNotExist,
  kKeyValueWrong,
  kUpdateKeySucceed,
  kUpdateKeyFailed,
  kUpdateKeyFunFailed,
  kDelKeySucceed,
  kDelKeyFailed,
  kDelKeyFunFailed,
};

class DBInterface {
public:
  // sets {key, value} if key does not exist, returns false on failure.
  virtual DBResult SetKeyValue(const std::string &key, const std::string &value, std::chrono::milliseconds expire) = 0;

  // sets {key, value} if key does not exist or value matches, returns false on failure.
  virtual DBResult UpdateExpire(const std::string &key, const std::string &value, std::chrono::milliseconds expire) = 0;

  // deletes {key, value} if value matches, returns false on failure.
  virtual DBResult DelKeyValue(const std::string &key, const std::string &value) = 0;

  virtual ~DBInterface() = default;
};

} // namespace dlm

#endif // _DLM_DB_INTERFACE_H_
