#include <iostream>
#include <vector>
#include <set>
#include <chrono>
#include <fstream>
#include "ReputationDynamics.hpp"
#include "Game.hpp"


std::vector<ActionRule> ActionRuleCandidates(const ReputationDynamics& rd) {
  // iteration over possible actions
  ActionRule base(511);  // use AllC as a baseline
  std::vector< std::pair<Reputation,Reputation> > free_pairs;
  for (int i = 0; i < 3; i++) {
    const Reputation X = static_cast<Reputation>(i);
    for (int j = 0; j < 3; j++) {
      const Reputation Y = static_cast<Reputation>(j);
      if (X == Reputation::G && Y == Reputation::G) continue;  // GG => C is fixed
      if (rd.RepAt(X, Y, Action::C) == rd.RepAt(X, Y, Action::D)) {
        // When reputation remains same for both actions, there is no reason to cooperate
        base.SetAction(X,Y, Action::D);
      }
      else {
        free_pairs.emplace_back( std::make_pair(X,Y) );
      }
    }
  }

  std::vector<ActionRule> ans;
  size_t n_pairs = free_pairs.size();
  for (size_t i = 0; i < (1ul<<n_pairs); i++) {
    ActionRule ar = base.Clone();
    for (size_t n = 0; n < n_pairs; n++) {
      Reputation rep_donor = free_pairs[n].first;
      Reputation rep_recip = free_pairs[n].second;
      Action act = static_cast<Action>( (i & (1ul << n)) >> n );
      ar.SetAction(rep_donor, rep_recip, act);
    }
    ans.push_back(ar);
  }
  return ans;
}

int main() {
  auto start = std::chrono::system_clock::now();

  const double mu_e = 0.02, mu_a = 0.02, benefit = 1.2, cost = 1.0;

  uint64_t total_count = 0ull, ess_count = 0ull;
  std::vector<uint64_t> ESS_ids;

  // when GGC => G and GGD => B are fixed, there are 3^16 = 43046721 types of reputation dynamics:
  // Top most two bits are fixed: 2*3^17 + 0*3^16 = 258280326
  uint64_t fixed_rep = 258280326ull;
  for (size_t i = 0; i < 43046721; i += 500000) {
    if (i % 10000 == 0) { std::cerr << i << std::endl; }
    ReputationDynamics rd(fixed_rep + i);
    assert(rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G);
    assert(rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B);

    std::vector<ActionRule> act_rules = ActionRuleCandidates(rd);
    for (const ActionRule& ar: act_rules) {
      total_count++;
      Game g(mu_e, mu_a, rd, ar);
      if (g.IsESS(benefit, cost)) {
        ess_count++;
        ESS_ids.push_back(g.ID());
        std::cout << "ESS is found: " << g.Inspect();
      }
    }
  }

  std::cout << "ESS / total : " << ess_count << " / " << total_count << std::endl;
  std::sort(ESS_ids.begin(), ESS_ids.end());
  std::ofstream fout("ESS_ids");
  for (uint64_t x: ESS_ids) { fout << x << "\n"; }
  fout.close();

  auto end = std::chrono::system_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
  std::cout << "Elapsed time: " << elapsed / 1000.0 << std::endl;

  return 0;
}
