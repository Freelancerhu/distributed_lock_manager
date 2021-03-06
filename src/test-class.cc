#include "test-class.h"

#include "cpp_redis/cpp_redis"

#include <iostream>
#include <string>
#include <vector> // client.del()
#include <chrono>


namespace dlm {
  TestClass::TestClass(const std::string &ip, uint16_t port) {
    //! Enable logging
    client.connect(ip, port,
      [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
      if (status == cpp_redis::client::connect_state::dropped) {
        std::cout << "client disconnected from " << host << ":" << port << std::endl;
      }
    });
  }

  void TestClass::Test() {
    std::chrono::milliseconds e{ 10000 };
    client.set_advanced("hu", std::to_string(25), false, false,
      true, int(e.count()), true, false, [](cpp_redis::reply& reply) {
      std::cout << "hu" << " : " << 25 << " : " << 5000000 << " : " << reply << std::endl;
      if (reply.is_null() == true) std::cout << "has been setted" << std::endl;
      else if (reply.ok() == true) std::cout << "success" << std::endl;
      else std::cout << "fail" << std::endl;
    });

    client.pexpire("hu", int(e.count()), [](cpp_redis::reply& reply) {
      std::cout << "hu" << " : " << 25 << " : " << 15000 << " : " << reply << std::endl;
      if (reply.as_integer() == int64_t(0)) {
        std::cout << "fail" << std::endl;
      }
      else if (reply.as_integer() == int64_t(1)) {
        std::cout << "succcess" << std::endl;
      }
    });

    auto exi = client.get("x");

    client.sync_commit();
    std::string t = exi.get().as_string();
    std::cout << "t = " << t << std::endl;
    if (t == "1")
      std::cout << "exi.get() : yyyy" << std::endl;
    else std::cout << "exi : noop" << std::endl;

    auto del = client.del(std::vector<std::string>{"he"});

    client.sync_commit();
    //std::string d = del.get().as_string();
    std::cout << "d = " << del.get().is_integer() << std::endl;


    std::chrono::milliseconds span(1000); 
    
    std::future<cpp_redis::reply> foo = client.get("hea");
    client.sync_commit();
    std::cout << "valid() : " <<  foo.valid() << std::endl;
    

    if (foo.valid()) {
      std::cout << foo.get().is_string() << std::endl;
      std::cout << "g" << std::endl;
    }
    else
      std::cout << "fuckin shit" << std::endl;
    std::cout << "adfasf" << std::endl;


  
  
  
  }

  std::string TestClass::equ(const std::string &key, uint16_t val) {
    //this.get (const std::string &key, const reply_callback_t &reply_callback)
    std::string tem = "";
    //std::future<cpp_redis::reply> cget = client.get("y");
    //client.get("y").wait();
    tem = client.get("y").get().as_string();
    std::cout << "??" << tem << std::endl;
 
  /*  client.get(key, [&tem](cpp_redis::reply& reply) {
      std::cout << "eql ? " << reply << std::endl;
      std::cout << reply.is_string() << std::endl;
      return reply.as_string();
    });*/
    return "ffffaiiilll";
  }


} // namespace dlm
