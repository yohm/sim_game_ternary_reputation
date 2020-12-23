#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>
#include "omp.h"
#include "mpi.h"
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

std::pair<std::vector<uint64_t>, uint64_t> find_ESSs(const ReputationDynamics& rd) {
  const double mu_e = 0.02, mu_a = 0.02, benefit = 1.2, cost = 1.0;
  const double coop_prob_th = 0.9;

  std::vector<uint64_t> ess_ids;
  uint64_t num_total = 0ull;
  std::vector<ActionRule> act_rules = ActionRuleCandidates(rd);
  for (const ActionRule& ar: act_rules) {
    num_total++;
    Game g(mu_e, mu_a, rd, ar);
    if (g.ResidentCoopProb() > coop_prob_th && g.IsESS(benefit, cost)) {
      ess_ids.push_back(g.ID());
      // std::cout << "ESS is found: " << g.Inspect();
    }
  }
  return std::make_pair(ess_ids, num_total);
}


int main(int argc, char *argv[]) {

  // MPI initialization
  MPI_Init(&argc, &argv);
  int _my_rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &_my_rank);
  int _num_procs = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &_num_procs);
  const int my_rank = _my_rank, num_procs = _num_procs;

  auto start = std::chrono::system_clock::now();


  uint64_t total_count = 0ull, ess_count = 0ull;
  std::vector<uint64_t> ESS_ids;

  // when GGC => G and GGD => B are fixed, there are 3^16 = 43,046,721 types of reputation dynamics:
  // Top most two bits are fixed: 2*3^17 + 0*3^16 = 258280326
  const uint64_t fixed_rep = 258280326ull;
  const uint64_t num_rep_dynamics = 43046721ull;

  const uint64_t num_rep_dynamics_per_rank = std::ceil( (double)num_rep_dynamics / num_procs);
  const uint64_t start_idx = my_rank * num_rep_dynamics_per_rank;
  const uint64_t end_idx = (my_rank == num_procs-1) ? num_rep_dynamics : (my_rank + 1) * num_rep_dynamics_per_rank;
  assert(start_idx < num_rep_dynamics);

  #pragma omp parallel for shared(total_count,ess_count,ESS_ids) default(none) schedule(dynamic)
  for (size_t i = start_idx; i < end_idx; i += 500000) {
    int th = omp_get_thread_num();
    int num_threads = omp_get_num_threads();
    if (true) { std::cerr << i << ' ' << th << '/' << num_threads << " : " << my_rank << '/' << num_procs << std::endl; }
    ReputationDynamics rd(fixed_rep + i);
    assert(rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G);
    assert(rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B);

    auto ans = find_ESSs(rd);
    #pragma omp atomic update
    total_count += ans.second;
    #pragma omp atomic update
    ess_count += ans.first.size();
    #pragma omp critical
    ESS_ids.insert(ESS_ids.end(), ans.first.begin(), ans.first.end());
  }

  std::cout << "ESS / total : " << ess_count << " / " << total_count << " at " << my_rank << " / " << num_procs << std::endl;
  std::sort(ESS_ids.begin(), ESS_ids.end());

  std::ostringstream os;
  char buffer[20];
  std::snprintf(buffer, sizeof(buffer), "ESS_ids_%06d", my_rank);
  std::ofstream fout(buffer);
  for (uint64_t x: ESS_ids) { fout << x << "\n"; }
  fout.close();

  auto end = std::chrono::system_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
  std::cout << "Elapsed time: " << elapsed / 1000.0 << " at " << my_rank << " / " << num_procs << std::endl;

  MPI_Finalize();

  return 0;
}
