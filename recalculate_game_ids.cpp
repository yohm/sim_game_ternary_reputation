#include <iostream>
#include <vector>
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
  const size_t N = game_ids.size();
  std::vector<uint64_t > new_game_ids(N, 0ull);
  std::vector<double> coop_probs(N, 0.0);
  std::vector<std::array<double,3> > hs(N);

  #pragma omp parallel for shared(game_ids,new_game_ids,coop_probs,hs)
  for (size_t i = 0; i < N; i++) {
    if (i % 10'000 == 0) { std::cerr << i << " / " << game_ids.size() << std::endl; }
    Game g(mu_e, mu_a, game_ids[i]);
    // std::cerr << g.rep_dynamics.ID() << ' ' << (game_ids[i] >> 9ull) << std::endl;
    // rep_ids.push_back( g.rep_dynamics.ID() );
    double cprob = g.ResidentCoopProb();
    auto h = g.ResidentEqReputation();

    std::array<int,3> map;
    if (h[0] >= h[1] && h[0] >= h[2]) {
      // 0: G
      Reputation G = static_cast<Reputation>(0);
      Reputation B = g.rep_dynamics.RepAt(G, G, Action::D);
      if (static_cast<int>(B) == 1) {
        map = {2,0,1};
      }
      else if (static_cast<int>(B) == 2) {
        map = {2,1,0};
      }
      else { throw std::runtime_error("must not happen"); }
    }
    else if (h[1] >= h[0] && h[1] >= h[2]) {
      // 1: G
      Reputation G = static_cast<Reputation>(1);
      Reputation B = g.rep_dynamics.RepAt(G, G, Action::D);
      if (static_cast<int>(B) == 0) {
        map = {0,2,1};
      }
      else if (static_cast<int>(B) == 2) {
        map = {1,2,0};
      }
      else { throw std::runtime_error("must not happen"); }
    }
    else if (h[2] >= h[0] && h[2] >= h[1]) {
      // 2: G
      Reputation G = static_cast<Reputation>(2);
      Reputation B = g.rep_dynamics.RepAt(G, G, Action::D);
      if (static_cast<int>(B) == 0) {
        map = {0,1,2};
      }
      else if (static_cast<int>(B) == 1) {
        map = {1,0,2};
      }
      else { throw std::runtime_error("must not happen"); }
    }
    else { throw std::runtime_error("must not happen"); }

    ReputationDynamics new_repd = g.rep_dynamics.Permute(map);
    ActionRule new_ar = g.resident_ar.Permute(map);
    Game new_g(mu_e, mu_a, new_repd, new_ar);

    new_game_ids[i] = new_g.ID();
    coop_probs[i] = cprob;
    std::array<double,3> new_h = {0.0};
    for (size_t n = 0; n < 3; n++) { hs[i][ map[n] ] = h[n]; }
    /*
    if (h[0] < 0.8 && h[1] < 0.8 && h[2] < 0.8 && cprob > 0.95) {
      std::cout << game_ids[i] << ' ' << cprob << ' ' << h[0] << ' ' << h[1] << ' ' << h[2] << std::endl;
      std::cout << g.Inspect();
    }
     */
  }

  for (size_t i = 0; i < N; i++) {
    std::cout << game_ids[i] << ' ' << new_game_ids[i] << ' ' << coop_probs[i] << ' ' << hs[i][0] << ' ' << hs[i][1] << ' ' << hs[i][2] << std::endl;
  }

  return 0;
}