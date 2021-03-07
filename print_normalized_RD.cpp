#include <iostream>
#include <fstream>
#include "Strategy.hpp"


int main( int argc, char* argv[]) {

  uint64_t max = 387'420'489ull;  // 3**18

  std::vector<uint64_t> ans;
  for (uint64_t i = 0; i < max; i++) {
    if (i % 1'000'000 == 0) { std::cerr << i << std::endl; }
    ReputationDynamics rd(i);
    if (rd.Normalized().first.ID() == i) {
      ans.push_back(i);
    }
  }

  for (uint64_t a : ans) {
    std::cout << a << "\n";
  }
  std::cerr << ans.size() << std::endl;

  return 0;
}