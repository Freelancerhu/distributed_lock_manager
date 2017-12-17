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

  client.connect("127.0.0.1", 6379, [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
      if (status == cpp_redis::client::connect_state::dropped) {
        std::cout << "client disconnected from " << host << ":" << port << std::endl;
      }
    });

  // same as client.send({ "SET", "hello", "42" }, ...)
  client.set("hello", "42", [](cpp_redis::reply& reply) {
      std::cout << "set hello 42: " << reply << std::endl;
      // if (reply.is_string())
      //   do_something_with_string(reply.as_string())
  });

  client.sync_commit();

#ifdef _WIN32
  WSACleanup();
#endif /* _WIN32 */

  std::cout << __func__ << " ended" << std::endl;
}

} // namespace hiredis_cc

#endif // _DLM_REDIS_CC_DEBUG_H_
