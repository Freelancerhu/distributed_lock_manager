#pragma once
#ifndef _DLM_DB_REDIS_H_
#define _DLM_DB_REDIS_H_

#include "cpp_redis/cpp_redis"

#include <chrono>
#include <string>
#include <vector>

#include "db-interface.h"

namespace dlm {

  class DBRedis : public DBInterface {
  public:
    // DBRedis();
    
    DBRedis(const std::string &ip = "127.0.0.1", uint16_t port = 6379);

    // sets {key, value} if key does not exist, returns false on failure.
    virtual DBResult SetKeyValue(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire);

    // sets {key, value} if key does not exist or value matches, returns false on failure.
    virtual DBResult UpdateExpire(const std::string &key, const std::string &value, const std::chrono::milliseconds &expire);

    // deletes {key, value} if value matches, returns false on failure.
    virtual DBResult DelKeyValue(const std::string &key, const std::string &value);

    //select another DB
    virtual DBResult SelectDB(const int &index);

    virtual ~DBRedis();

  private:
    cpp_redis::client client;
  };
} // namespace dlm

#endif //_DLM_DB_REDIS_H_




