#include <iostream>

#include "redis-cc/hiredis-cc-debug.h"

#include <boost/regex.hpp>
#include <iostream>
#include <string>

int main() {
  redis_cc::test();
  std::clog << __func__ << std::endl;

  std::string line;
  boost::regex pat( "^Subject: (Re: |Aw: )*(.*)" );

  while (std::cin) {
    std::getline(std::cin, line);
    boost::smatch matches;
    if (boost::regex_match(line, matches, pat))
      std::cout << matches[2] << std::endl;
  }

  return 0;
}
