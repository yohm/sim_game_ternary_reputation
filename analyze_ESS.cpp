#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>

#include "ReputationDynamics.hpp"
#include "Game.hpp"


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

  std::vector<uint64_t> game_ids;

  while(fin) {
    uint64_t i;
    fin >> i;
    if (fin) { game_ids.push_back(i); }
  }

  std::cerr << "Loaded " << game_ids.size() << " games" << std::endl;
  const double mu_e = 0.02, mu_a = 0.02;
  std::vector<uint64_t> rep_ids;
  for (size_t i = 0; i < game_ids.size(); i++) {
    if (i % 10'000 == 0) { std::cerr << i << " / " << game_ids.size() << std::endl; }
    Game g(mu_e, mu_a, game_ids[i]);
    rep_ids.push_back( g.rep_dynamics.ID() );
    double cprob = g.ResidentCoopProb();
    if (cprob > 0.8) { std::cerr << g.ID() << ' ' << cprob << std::endl; }
  }
  std::sort(rep_ids.begin(), rep_ids.end());
  rep_ids.erase( std::unique(rep_ids.begin(), rep_ids.end()), rep_ids.end() );

  std::cout << "num_ESSs: " << game_ids.size() << std::endl
            << "num_RepDynamics: " << rep_ids.size() << std::endl;

  return 0;
}