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

std::vector<uint64_t> LoadInputFiles(const char* fname, int my_rank, int num_procs) {
  std::ifstream fin(fname);
  if (!fin) {
    std::cerr << "Failed to open file " << fname << std::endl;
    MPI_Abort(MPI_COMM_WORLD, 2);
  }

  std::vector<uint64_t> rep_ids;

  while(fin) {
    uint64_t i;
    fin >> i;
    if (fin) { rep_ids.emplace_back(i); }
  }

  std::vector<uint64_t> my_rep_ids;
  for (size_t i = my_rank; i < rep_ids.size(); i+=num_procs) {
    my_rep_ids.emplace_back( rep_ids[i] );
  }

  return std::move(my_rep_ids);
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

  if (argc != 2) {
    std::cerr << "invalid number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <reputation dynamics id list>" << std::endl;
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  const std::vector<uint64_t> repd_ids = LoadInputFiles(argv[1], my_rank, num_procs);

  uint64_t total_count = 0ull, ess_count = 0ull;
  std::vector<uint64_t> ESS_ids;

  #pragma omp parallel for shared(total_count,ess_count,ESS_ids) default(none) schedule(dynamic)
  for (size_t i = 0; i <repd_ids.size(); i++) {
    int th = omp_get_thread_num();
    int num_threads = omp_get_num_threads();
    // if (true) { std::cerr << repd_ids[i] << ' ' << th << '/' << num_threads << " : " << my_rank << '/' << num_procs << std::endl; }
    ReputationDynamics rd(repd_ids[i]);

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

  uint64_t total_count_sum = 0ull, ess_count_sum = 0ull;
  MPI_Reduce(&total_count, &total_count_sum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&ess_count, &ess_count_sum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  if (my_rank == 0) {
    std::cout << "SUM ESS / SUM TOTAL : " << ess_count_sum << " / " << total_count_sum << std::endl;
  }

  MPI_Finalize();

  return 0;
}
