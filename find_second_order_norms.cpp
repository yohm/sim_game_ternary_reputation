#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>
#include <icecream.hpp>

#include "Strategy.hpp"


struct Input {
  Input(uint64_t _gid, double _cprob, double h0, double h1, double h2, double b_lower, double b_upper) : gid(_gid), c_prob(_cprob), h({h0,h1,h2}), b_range({b_lower, b_upper}) {};
  uint64_t gid;
  double c_prob;
  std::array<double,3> h;
  std::array<double,2> b_range;
};

int main(int argc, char* argv[]) {

  if (argc < 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file> ..." << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  for (int c = 1; c < argc; c++) {
    std::ifstream fin(argv[c]);
    std::cerr << "reading " << argv[c] << std::endl;
    if (!fin) {
      std::cerr << "Failed to open file " << argv[c] << std::endl;
      throw std::runtime_error("failed to open file");
    }

    std::vector<Input> inputs;
    while(fin) {
      uint64_t gid;
      double c_prob,h0,h1,h2,b_lower,b_upper;
      fin >> gid >> c_prob >> h0 >> h1 >> h2 >> b_lower >> b_upper;
      if (fin) {
        inputs.emplace_back(gid, c_prob, h0, h1, h2, b_lower, b_upper);
      }
    }

    for (size_t i = 0; i < inputs.size(); i++) {
      // if (inputs.size() > 20 && i % (inputs.size()/20) == 0) { std::cerr << "progress: " << (i*100)/inputs.size() << " %" << std::endl; }
      Input input = inputs[i];
      Strategy str(input.gid);
      if ( str.IsSecondOrder() ) {
        std::cerr << str.Inspect() << std::endl;
      }
    }
  }

  return 0;
}