
#include "test-class.h"
#include "db-interface.h"
#include "db-redis.h"
#include "dmutex.h"
#include <Winsock2.h>
#include <iostream>
#include <vector>
#include <thread>

int main() {
  WORD version = MAKEWORD(2, 2);
  WSADATA data;

  if (WSAStartup(version, &data) != 0) {
    std::cerr << "WSAStartup() failure" << std::endl;
    return -1;
  }
  std::cout << "dlm-example.cpp starts" << std::endl;

  //dlm::DBRedis testClient;
  //testClient.dlm::DBRedis::DBInterfaceTest();
  std::vector<int> temp_vec = { 1, 2, 3, 4, 5 };
  dlm::DMutex DM1(temp_vec);
  dlm::DMutex DM2(temp_vec);
  std::thread t1([&DM1]() {
    DM1.Lock("test1", "t1");
  });
  std::thread t2([&DM2]() {
    DM2.Lock("test1", "t2");
  });

  t1.join();
  t2.join();
  std::cout << "dlm-example.cpp ends" << std::endl;

  WSACleanup();
  return 0;
}
