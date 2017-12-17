#include <iostream>

#include "redis-cc/hiredis-cc-debug.h"

int main() {
  redis_cc::test();
  std::clog << __func__ << std::endl;
  return 0;
}
