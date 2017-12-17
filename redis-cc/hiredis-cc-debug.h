#ifndef _DLM_REDIS_CC_DEBUG_H_
#define _DLM_REDIS_CC_DEBUG_H_

#include "cpp_redis/cpp_redis"

#include <iostream>


#ifdef _WIN32
#include <Winsock2.h>
#endif /* _WIN32 */

namespace redis_cc {
void test() {
#ifdef _WIN32
  //! Windows netword DLL init
  WORD version = MAKEWORD(2, 2);
  WSADATA data;

  if (WSAStartup(version, &data) != 0) {
    std::cerr << "WSAStartup() failure" << std::endl;
    return ;
  }
#endif /* _WIN32 */

  cpp_redis::client client;

#ifdef _WIN32
  WSACleanup();
#endif /* _WIN32 */

}

} // namespace hiredis_cc

#endif // _DLM_REDIS_CC_DEBUG_H_
