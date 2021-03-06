#include "test-class.h"
#include "db-interface.h"
#include "db-redis.h"
#include "dmutex.h"
#include <Winsock2.h>
#include <iostream>
#include <vector>
#include <thread>


std::chrono::milliseconds GetCurrentMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch());
}

int main() {
  WORD version = MAKEWORD(2, 2);
  WSADATA data;

  if (WSAStartup(version, &data) != 0) {
    std::cerr << "WSAStartup() failure" << std::endl;
    return -1;
  }
  std::cout << "dlm-example.cpp starts" << std::endl;
  std::vector<int> temp_vec1 = { 1,2,3,4,5 };

  dlm::DMutex DM1("test1", temp_vec1, std::chrono::milliseconds{ 10000 }, std::chrono::milliseconds{ 5500 });

  uint64_t start_time_milliseconds = GetCurrentMilliseconds().count();
  std::chrono::milliseconds expire{ 1000 };
  int count = 0;
  int sum = 0;
  std::mutex gmutex;
  while (GetCurrentMilliseconds().count() - start_time_milliseconds <= expire.count()) {
    DM1.lock();
    uint64_t s = GetCurrentMilliseconds().count();
    //gmutex.lock();
    count++;
    DM1.unlock();
    uint64_t e = GetCurrentMilliseconds().count();
    std::cout << e - s << std::endl;
    //gmutex.unlock();
  }

  //dlm::DMutex DM2("test1",temp_vec1, std::chrono::milliseconds{ 10000 }, std::chrono::milliseconds{ 5000 });
  //dlm::DMutex DM3("test3", temp_vec1, std::chrono::milliseconds{ 10000 }, std::chrono::milliseconds{ 5000 });
  //dlm::DMutex DM4("test4", temp_vec1, std::chrono::milliseconds{ 10000 }, std::chrono::milliseconds{ 5000 });
  //dlm::DMutex DM5("test5", temp_vec1, std::chrono::milliseconds{ 10000 }, std::chrono::milliseconds{ 5000 });

  //std::thread t1([&DM1]() {
  //  DM1.lock();
  //  std::this_thread::sleep_for(std::chrono::milliseconds{ 20000 });
  //  DM1.unlock();
  //});
  //std::thread t2([&DM2]() {
  //  bool res = DM2.try_lock(std::chrono::milliseconds{ 80000 });
  //  //bool res = DM2.try_lock();
  //  std::cout << "t2 lock res = " << res << std::endl;
  //  std::this_thread::sleep_for(std::chrono::milliseconds{ 30000 });
  //  DM2.unlock();
  //});
  //std::thread t3([&DM3]() {
  //  DM3.lock();
  //  std::this_thread::sleep_for(std::chrono::milliseconds{ 20000 });
  //  DM3.unlock();
  //});
  //std::thread t4([&DM4]() {
  //  DM4.lock();
  //  std::this_thread::sleep_for(std::chrono::milliseconds{ 20000 });
  //  DM4.unlock();
  //});
  //std::thread t5([&DM5]() {
  //  DM5.lock();
  //  std::this_thread::sleep_for(std::chrono::milliseconds{ 20000 });
  //  DM5.unlock();
  //});
  //t1.join();
  //t2.join();
  //t3.join();
  //t4.join();
  //t5.join();
  std::cout << "count = " << count << std::endl;
  std::cout << "dlm-example.cpp ends" << std::endl;
  //std::this_thread::sleep_for(std::chrono::milliseconds{ 69990000 });
  WSACleanup();
  std::cout << "the fin" << std::endl;
  return 0;
}
