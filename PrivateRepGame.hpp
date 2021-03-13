//
// Created by Yohsuke Murase on 2021/03/13.
//

#ifndef SIM_GAME_TERNARY_REPUTATION_MODEL_PRIVATEREPGAME_HPP
#define SIM_GAME_TERNARY_REPUTATION_MODEL_PRIVATEREPGAME_HPP

#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <map>
#include <utility>
#include "Strategy.hpp"

class PrivateRepGame {
  public:
  using population_t = std::vector<std::pair<uint64_t, size_t>>;  // vector of StrategyID & its size
  PrivateRepGame(const population_t& pop, uint64_t _seed) : population(pop), rnd(_seed), uni(0.0, 1.0) {
    for (size_t s = 0; s < population.size(); s++) {
      auto kv = population[s];
      for (size_t i = 0; i < kv.second; i++) {
        strategies.emplace_back(kv.first);
        strategy_index.emplace_back(s);
      }
    }
    N = strategies.size();
    M.resize(N);
    for (size_t i = 0; i < N; i++) { M[i].resize(N, Reputation::G); }  // initialize with Good reputation
    ResetCoopCount();
  }

  void Update(size_t t_max, double q, double epsilon) {
    for (size_t t = 0; t < t_max; t++) {
      // randomly choose donor & recipient
      size_t donor = static_cast<size_t>(R01() * N);
      size_t recip = static_cast<size_t>(R01() * N);
      while (recip == donor) { recip = static_cast<size_t>(R01() * N); }

      Action action = strategies[donor].Act( M[donor][donor], M[donor][recip] );

      size_t s1 = strategy_index[donor], s2 = strategy_index[recip];
      if (action == Action::C) { coop_count[s1][s2][0]++; }
      coop_count[s1][s2][1]++;

      // updating the donor's reputation
      for (size_t obs = 0; obs < N; obs++) {
        if (obs == donor || obs == recip || R01() < q) {  // observe with probability q
          Action a_obs = (R01() < 1.0 - epsilon) ? action : FlipAction(action);
          Reputation new_rep = strategies[obs].Assess(M[obs][donor], M[obs][recip], a_obs);
          M[obs][donor] = new_rep;
        }
      }
    }
  }

  // for each strategy combination, count the number of B,N,G reputations
  using rep_count_t = std::map< std::pair<uint64_t,uint64_t>, std::array<size_t,3>>;

  void CountUpGoodRepSpeciesWise(rep_count_t& ans) const {
    size_t i_begin = 0;
    for (auto kv_i : population) {
      uint64_t species_i = kv_i.first;
      size_t i_end = i_begin + kv_i.second;
      size_t j_begin = 0;
      for (auto kv_j : population) {
        uint64_t species_j = kv_j.first;
        size_t j_end = j_begin + kv_j.second;
        // count 1 between [[i_begin,i_end), [j_begin, j_end)]
        auto& counter = ans[{species_i, species_j}];
        for (size_t i = i_begin; i < i_end; i++) {
          for (size_t j = j_begin; j < j_end; j++) {
            int r = static_cast<int>(M[i][j]);
            counter[r]++;
          }
        }
        //
        j_begin = j_end;
      }
      i_begin = i_end;
    }
  }

  // key : pair of [donor_id, recipient_id]
  // value: [number of cooperation] / [number of actions (cooperation + defection)
  std::map< std::pair<uint64_t,uint64_t>, std::array<size_t,2>> GetCoopCount() const {
    std::map< std::pair<uint64_t, uint64_t>, std::array<size_t,2>> ans;
    for (size_t si = 0; si < coop_count.size(); si++) {
      uint64_t si_id = population[si].first;
      for (size_t sj = 0; sj < coop_count[si].size(); sj++) {
        uint64_t sj_id = population[sj].first;
        auto key = std::make_pair(si_id, sj_id);
        ans[key] = coop_count[si][sj];
      }
    }
    return ans;
  }

  void ResetCoopCount() {
    coop_count.clear();
    coop_count.resize(population.size());
    for (auto& ci : coop_count) {
      ci.resize(population.size());
      for (auto& cij: ci) {
        cij[0] = 0ul;
        cij[1] = 0ul;
      }
    }
  }

  void PrintM(std::ostream& out = std::cerr) const {
    for (size_t i = 0; i < M.size(); i++) {
      for (size_t j = 0; j < M[i].size(); j++) {
        out << M[i][j] << ' ';
      }
      out << "\n";
    }
  }

  private:
  const population_t population;
  size_t N;
  std::mt19937_64 rnd;
  std::uniform_real_distribution<double> uni;
  std::vector<Strategy> strategies;
  std::vector<size_t> strategy_index;  // strategy_index[i] == j : i-th player is a j-th species
  std::vector<std::vector<Reputation> > M;
  std::vector< std::vector< std::array<size_t,2> >> coop_count;
  double R01() { return uni(rnd); }
};

#endif //SIM_GAME_TERNARY_REPUTATION_MODEL_PRIVATEREPGAME_HPP
