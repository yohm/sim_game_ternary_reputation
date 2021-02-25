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
#include <caravan.hpp>


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

class Output {
  public:
  Output(uint64_t _gid, double _cprob, const std::array<double,3>& _h) : gid(_gid), cprob(_cprob), h(_h) {};
  Output(const Game& g) : gid(g.ID()), cprob(g.ResidentCoopProb()), h(g.ResidentEqReputation()) {};
  uint64_t gid;
  double cprob;
  std::array<double,3> h;
  bool operator<(const Output& rhs) const { return gid < rhs.gid; }
};

std::pair<std::vector<Output>, uint64_t> find_ESSs(const ReputationDynamics& rd) {
  const double mu_e = 0.02, mu_a = 0.02, benefit = 1.2, cost = 1.0;
  const double coop_prob_th = 0.9;

  std::vector<Output> ess_ids;
  uint64_t num_total = 0ull;
  std::vector<ActionRule> act_rules = ActionRuleCandidates(rd);
  for (const ActionRule& ar: act_rules) {
    num_total++;
    Game g(mu_e, mu_a, rd, ar);
    if (g.ResidentCoopProb() > coop_prob_th && g.IsESS(benefit, cost)) {
      Game new_g = g.NormalizedGame();
      ess_ids.emplace_back(new_g);
    }
  }
  return std::make_pair(ess_ids, num_total);
}

std::vector<uint64_t> LoadInputFiles(const char* fname) {
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

  return std::move(rep_ids);
}

std::vector<Output> SearchRepDsOpenMP(const std::vector<uint64_t>& repd_ids) {
  int num_threads;
  #pragma omp parallel shared(num_threads) default(none)
  { num_threads = omp_get_num_threads(); };

  std::vector<std::vector<Output>> outs_thread(num_threads);
  // std::vector<uint64_t> ESS_ids;

  #pragma omp parallel for shared(outs_thread,repd_ids) default(none) schedule(dynamic)
  for (size_t i = 0; i <repd_ids.size(); i++) {
    int th = omp_get_thread_num();
    ReputationDynamics rd(repd_ids[i]);

    auto ans = find_ESSs(rd);
    outs_thread[th].insert(outs_thread[th].end(), ans.first.begin(), ans.first.end());
  }

  std::vector<Output> outs;
  for (const auto& o: outs_thread) {
    outs.insert(outs.end(), o.begin(), o.end());
  }
  return std::move(outs);
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

  std::ofstream fout;
  using json = nlohmann::json;

  std::function<void(caravan::Queue&)> on_init = [&argv,&fout](caravan::Queue& q) {
    const std::vector<uint64_t> repd_ids = LoadInputFiles(argv[1]);
    fout.open("ESS_ids");
    for(uint64_t id : repd_ids) {
      json input = id;
      q.Push(input);
    }
  };
  std::function<void(int64_t, const json&, const json&, caravan::Queue&)> on_result_receive = [&fout](int64_t task_id, const json& input, const json& output, caravan::Queue& q) {
    for (auto o: output) {
      IC(o.dump());
      fout << o.at(0).get<uint64_t>() << ' ' << o.at(1).get<double>() << ' ' << o.at(2).get<double>() << ' ' << o.at(3).get<double>() << ' ' << o.at(4).get<double>() << "\n";
    }
  };
  std::function<json(const json&)> do_task = [](const json& input) {
    uint64_t repd_id = input.get<uint64_t>();
    ReputationDynamics rd(repd_id);
    auto ans = find_ESSs(rd);
    json result;
    for (const Output& out: ans.first) {
      result.emplace_back( std::make_tuple(out.gid, out.cprob, out.h[0], out.h[1], out.h[2]) );
    }
    return result;
  };

  caravan::Start(on_init, on_result_receive, do_task, MPI_COMM_WORLD, 384, 2);

  MPI_Finalize();

  return 0;
}
