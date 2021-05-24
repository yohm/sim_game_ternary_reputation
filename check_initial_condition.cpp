#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <array>
#include <icecream.hpp>
#include "Game.hpp"


bool Close(const std::array<double,3>& a1, const std::array<double,3>& a2, double tolerance = 1.0e-2) {
  return (std::abs(a1[0]-a2[0]) < tolerance) && (std::abs(a1[1]-a2[1]) < tolerance) && (std::abs(a1[2]-a2[2]) < tolerance);
}

struct input_t {
  uint64_t id;
  double coop_prob;
  std::array<double,3> h;
};

void LoadFile(const char* fname, std::vector<input_t>& inputs) {
  std::ifstream fin(fname);
  if (!fin) {
    std::cerr << "Failed to open file " << fname << std::endl;
    throw std::runtime_error("failed to open file");
  }

  while(fin) {
    input_t in;
    fin >> in.id >> in.coop_prob >> in.h[0] >> in.h[1] >> in.h[2];
    if (fin) {
      inputs.emplace_back(in);
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<input_t> inputs;
  LoadFile(argv[1], inputs);

  #pragma omp parallel for shared(inputs,std::cerr) default(none)
  for (size_t n = 0; n < inputs.size(); n++) {
    if (n % 1 == 0) { std::cerr << "progress: " << n << " / " << inputs.size() << std::endl; }
    const input_t& input = inputs[n];
    Game g(0.02, 0.02, input.id, input.coop_prob, input.h);

    const auto base = g.ResidentEqReputation();
    const int N = 5;
    const double N_inv = 1.0 / (double)N;
    for (int i = 0; i <= N; i++) {
      for (int j = 0; j <= (N-i); j++) {
        int k = N - i - j;
        auto a = g.CalcHStarFromInitialPoint({i*N_inv, j*N_inv, k*N_inv});
        if (!Close(a, base)) {
          IC(base, a, i, j, k);
          std::runtime_error("initial condition dependency is detected");
        }
      }
    }
  }
  return 0;
}
