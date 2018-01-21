#ifndef _DLM_TEST_CLASS_H_
#define _DLM_TEST_CLASS_H_

#include "cpp_redis/cpp_redis"

#include <string>

namespace dlm {
class TestClass {
public:
  TestClass(const std::string &ip = "127.0.0.1", uint16_t port = 6379);
  void Test();
  std::string equ(const std::string &key, uint16_t val);
  cpp_redis::client client;
};

} // namespace dlm

#endif // _DLM_TEST_CLASS_H_
