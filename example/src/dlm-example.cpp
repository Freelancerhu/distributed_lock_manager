
#include "test-class.h"
#include "DbRedisInterface.h"

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
  dlm::DbRedisInterface testClient;
  testClient.dlm::DbRedisInterface::DBInterfaceTest();

  WSACleanup();
  return 0;
}
