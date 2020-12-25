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

  {
    std::vector<uint64_t> rep_ids;
    for (size_t i = 0; i < game_ids.size(); i++) {
      rep_ids.emplace_back(game_ids[i] >> 9ull);
    }
    std::sort(rep_ids.begin(), rep_ids.end());
    rep_ids.erase( std::unique(rep_ids.begin(), rep_ids.end()), rep_ids.end() );

    std::cout << "num_ESSs: " << game_ids.size() << std::endl
              << "num_RepDynamics: " << rep_ids.size() << std::endl;

    /*
    std::vector< std::array<size_t,3> > histo(18, {0,0,0});
    for (uint64_t id: rep_ids) {
      ReputationDynamics rd(id);
      for (size_t n = 0; n < 18; n++) {
        histo[n][static_cast<int>(rd.reputations[n])]++;
      }
    }
    for (size_t n = 0; n < 18; n++) {
      std::cout << n << " : " << histo[n][0] << ' ' << histo[n][1] << ' ' << histo[n][2] << std::endl;
    }
     */
  }


  const double mu_e = 0.02, mu_a = 0.02;
  for (size_t i = 0; i < game_ids.size(); i++) {
    if (i % 10'000 == 0) { std::cerr << i << " / " << game_ids.size() << std::endl; }
    Game g(mu_e, mu_a, game_ids[i]);
    // std::cerr << g.rep_dynamics.ID() << ' ' << (game_ids[i] >> 9ull) << std::endl;
    // rep_ids.push_back( g.rep_dynamics.ID() );
    double cprob = g.ResidentCoopProb();
    auto h = g.ResidentEqReputation();
    std::array<int,3> map;
    if (h[0] >= h[1] && h[1] >= h[2]) { map = {2,1,0}; }
    else if (h[0] >= h[2] && h[2] >= h[1]) { map = {2, 0, 1}; }
    else if (h[1] >= h[0] && h[0] >= h[2]) { map = {1, 2, 2}; }
    else if (h[1] >= h[2] && h[2] >= h[0]) { map = {0, 2, 1}; }
    else if (h[2] >= h[1] && h[1] >= h[0]) { map = {0, 1, 2}; }
    else if (h[2] >= h[0] && h[0] >= h[1]) { map = {1, 0, 2}; }
    else { throw std::runtime_error("must not happen"); }
    ReputationDynamics new_repd = g.rep_dynamics.Permute(map);
    ActionRule new_ar = g.resident_ar.Permute(map);
    Game new_g(mu_e, mu_a, new_repd, new_ar);
    std::array<double,3> new_h = {0.0};
    for (size_t i = 0; i < 3; i++) { new_h[ map[i] ] = h[i]; }
    std::cout << g.ID() << ' ' << new_g.ID() << ' ' << cprob << ' ' << new_h[0] << ' ' << new_h[1] << ' ' << new_h[2] << std::endl;
    /*
    if (h[0] < 0.8 && h[1] < 0.8 && h[2] < 0.8 && cprob > 0.95) {
      std::cout << game_ids[i] << ' ' << cprob << ' ' << h[0] << ' ' << h[1] << ' ' << h[2] << std::endl;
      std::cout << g.Inspect();
    }
     */
  }

  return 0;
}