
#include "test-class.h"
#include "db-redis.h"

#include <Winsock2.h>
#include <iostream>

int main() {
  WORD version = MAKEWORD(2, 2);
  WSADATA data;

  if (WSAStartup(version, &data) != 0) {
    std::cerr << "WSAStartup() failure" << std::endl;
    return -1;
  }
  std::cout << "dlm-example.cpp starts" << std::endl;
  dlm::DBRedis testClient;
  testClient.dlm::DBRedis::DBInterfaceTest();

  WSACleanup();
  return 0;
}
