#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>
#include <ctime>
#include "omp.h"
#include "mpi.h"
#include "Strategy.hpp"
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
  Output(uint64_t _gid, double _cprob, const std::array<double,3>& _h, const std::array<double,2>& _ess_range) : gid(_gid), cprob(_cprob), h(_h), ess_b_range(_ess_range) {};
  Output(const Game& g, const std::array<double,2>& _ess_range) : gid(g.ID()), cprob(g.ResidentCoopProb()), h(g.ResidentEqReputation()),
                                                                  ess_b_range(_ess_range) {};
  uint64_t gid;
  double cprob;
  std::array<double,3> h;
  std::array<double,2> ess_b_range;
  bool operator<(const Output& rhs) const { return gid < rhs.gid; }
};

struct Param {
  double mu_e, mu_a, benefit_lower_max, benefit_upper_min, coop_prob_th;
  Param(double _mu_e, double _mu_a, double _benefit_lower_max, double _benefit_upper_min, double _coop_prob_th) :
  mu_e(_mu_e), mu_a(_mu_a), benefit_lower_max(_benefit_lower_max), benefit_upper_min(_benefit_upper_min), coop_prob_th(_coop_prob_th) {};
};

std::pair<std::vector<Output>, uint64_t> find_ESSs(const ReputationDynamics& rd, const Param& prm) {
  std::vector<Output> ess_ids;
  uint64_t num_total = 0ull;
  std::vector<ActionRule> act_rules = ActionRuleCandidates(rd);
  for (const ActionRule& ar: act_rules) {
    num_total++;
    Game g(prm.mu_e, prm.mu_a, rd, ar);
    if (g.ResidentCoopProb() > prm.coop_prob_th) {
      auto b_range = g.ESS_Benefit_Range(prm.benefit_lower_max, prm.benefit_upper_min);
      if (b_range[0] < b_range[1]) {
        Game new_g = g.NormalizedGame();
        ess_ids.emplace_back(new_g, b_range);
      }
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

std::vector<Output> SearchRepDsOpenMP(const std::vector<uint64_t>& repd_ids, const Param& prm) {
  int num_threads;
  #pragma omp parallel shared(num_threads) default(none)
  { num_threads = omp_get_num_threads(); };

  std::vector<std::vector<Output>> outs_thread(num_threads);
  // std::vector<uint64_t> ESS_ids;

  #pragma omp parallel for shared(outs_thread,repd_ids,prm,std::cerr) default(none) schedule(dynamic)
  for (size_t i = 0; i <repd_ids.size(); i++) {
    int th = omp_get_thread_num();
    ReputationDynamics rd(repd_ids[i]);
    auto t1 = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    std::cerr << std::ctime(&t1) << ' ' << rd.ID() << " start" << std::endl;

    auto ans = find_ESSs(rd,prm);
    outs_thread[th].insert(outs_thread[th].end(), ans.first.begin(), ans.first.end());

    auto t2 = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    std::cerr << std::ctime(&t2) << ' ' << rd.ID() << " done" << std::endl;
  }

  std::vector<Output> outs;
  for (const auto& o: outs_thread) {
    outs.insert(outs.end(), o.begin(), o.end());
  }
  return std::move(outs);
}

using json = nlohmann::json;

Param BcastParameters(char* input_json_path) {
  std::vector<uint8_t> opt_buf;
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0) {
    json j;
    std::ifstream fin(input_json_path);
    if (!fin) {
      std::cerr << "failed to open " << input_json_path << "\n";
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fin >> j;
    opt_buf = std::move(json::to_msgpack(j));
    uint64_t size = opt_buf.size();
    MPI_Bcast(&size, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    MPI_Bcast(opt_buf.data(), opt_buf.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
  } else {
    uint64_t size = 0;
    MPI_Bcast(&size, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    opt_buf.resize(size);
    MPI_Bcast(opt_buf.data(), opt_buf.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
  }
  json j = json::from_msgpack(opt_buf);
  return Param(
    j.at("mu_e").get<double>(),
    j.at("mu_a").get<double>(),
    j.at("benefit_lower_max").get<double>(),
    j.at("benefit_upper_min").get<double>(),
    j.at("coop_prob_th").get<double>()
    );
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

  if (argc != 4) {
    std::cerr << "invalid number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <reputation dynamics id list> <input_json> <chunk size>" << std::endl;
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  Param prm = BcastParameters(argv[2]);
  const size_t chunk_size = std::stoul(argv[3]);

  std::ofstream fout;

  std::function<void(caravan::Queue&)> on_init = [&argv,chunk_size,&fout](caravan::Queue& q) {
    const std::vector<uint64_t> repd_ids = LoadInputFiles(argv[1]);
    fout.open("ESS_ids");

    json buf;
    for(uint64_t id : repd_ids) {
      buf.emplace_back(id);
      if (buf.size() == chunk_size) {
        q.Push(buf);
        buf.clear();
      }
    }
    if (!buf.empty()) {
      q.Push(buf);
    }
  };
  std::function<void(int64_t, const json&, const json&, caravan::Queue&)> on_result_receive = [&fout](int64_t task_id, const json& input, const json& output, caravan::Queue& q) {
    for (auto o: output) {
      fout << o.at(0).get<uint64_t>() << ' ' << o.at(1).get<double>() << ' ' << o.at(2).get<double>() << ' ' << o.at(3).get<double>() << ' ' << o.at(4).get<double>() << ' ' << o.at(5).get<double>() << ' ' << o.at(6).get<double>() << "\n";
    }
    size_t s = q.Size();
    if (s % 100 == 0) { std::cerr << "q.Size: " << s << std::endl; }
  };
  std::function<json(const json&)> do_task = [prm](const json& input) {
    std::vector<uint64_t> repd_ids;
    for (const auto in: input) {
      repd_ids.emplace_back( in.get<uint64_t>() );
    }
    std::vector<Output> outs = SearchRepDsOpenMP(repd_ids, prm);
    json result;
    for (const Output& out: outs) {
      result.emplace_back( std::make_tuple(out.gid, out.cprob, out.h[0], out.h[1], out.h[2], out.ess_b_range[0], out.ess_b_range[1]) );
    }
    return result;
  };

  caravan::Start(on_init, on_result_receive, do_task, MPI_COMM_WORLD);

  MPI_Finalize();

  return 0;
}
