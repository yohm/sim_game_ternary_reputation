#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>
#include <regex>
#include <omp.h>
#include <icecream.hpp>

#include "Strategy.hpp"
#include "Game.hpp"
#include "HistoNormalBin.hpp"


struct Input {
  Input(uint64_t _gid, double _cprob, double h0, double h1, double h2) : gid(_gid), c_prob(_cprob), h({h0,h1,h2}) {};
  uint64_t gid;
  double c_prob;
  std::array<double,3> h;
};

int main(int argc, char* argv[]) {

  if (argc != 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::ifstream fin(argv[1]);
  if (!fin) {
    std::cerr << "Failed to open file " << argv[1] << std::endl;
    throw std::runtime_error("failed to open file");
  }

  std::vector<Input> inputs;

  while(fin) {
    uint64_t gid;
    double c_prob,h0,h1,h2;
    fin >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      inputs.emplace_back(gid, c_prob, h0, h1, h2);
    }
  }

  using range_t = std::pair<double, double>;
  std::vector<range_t> ranges;

  for (size_t i = 0; i < inputs.size(); i++) {
    if (inputs.size() > 20 && i % (inputs.size()/20) == 0) { std::cerr << "progress: " << (i*100)/inputs.size() << " %" << std::endl; }
    Input input = inputs[i];

    Game g(0.001, 0.001, input.gid);
    double c_prob = g.ResidentCoopProb();
    range_t r = g.ESS_Benefit_Range();
    ranges.emplace_back(r);
  }

  IC(ranges);

  return 0;
}