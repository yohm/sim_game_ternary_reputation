#include <iostream>
#include <fstream>
#include <random>
#include <array>
#include <map>
#include <cstdint>
#include <icecream.hpp>
#include <mpi.h>
#include "Strategy.hpp"
#include <caravan.hpp>



class PrivateRepGame {
  public:
  using population_t = std::vector<std::pair<uint64_t, size_t>>;  // vector of StrategyID & its size
  PrivateRepGame(const population_t& pop, uint64_t _seed) : population(pop), rnd(_seed), uni(0.0, 1.0) {
    for (auto kv: population) {
      for (size_t i = 0; i < kv.second; i++) {
        strategies.emplace_back(kv.first);
      }
    }
    N = strategies.size();
    M.resize(N);
    for (size_t i = 0; i < N; i++) { M[i].resize(N, Reputation::G); }  // initialize with Good reputation
    RestCoopCount();
  }

  void Update(size_t t_max, double q, double epsilon) {
    for (size_t t = 0; t < t_max; t++) {
      // randomly choose donor & recipient
      size_t donor = static_cast<size_t>(R01() * N);
      size_t recip = static_cast<size_t>(R01() * N);
      while (recip == donor) { recip = static_cast<size_t>(R01() * N); }

      Action action = strategies[donor].Act( M[donor][donor], M[donor][recip] );

      auto key = std::make_pair(strategies[donor].ID(), strategies[recip].ID());
      if (action == Action::C) { coop_count[key][0]++; }
      coop_count[key][1]++;

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
  std::map< std::pair<uint64_t,uint64_t>, std::array<size_t,2>> GetCoopCount() const { return coop_count; }

  void RestCoopCount() {
    coop_count.clear();
    for (auto kv_i: population) {
      uint64_t species_i = kv_i.first;
      for (auto kv_j: population) {
        uint64_t species_j = kv_j.first;
        coop_count[{species_i,species_j}] = {0, 0};
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
  std::vector<std::vector<Reputation> > M;
  std::map<std::pair<uint64_t,uint64_t>, std::array<size_t,2>> coop_count;
  double R01() { return uni(rnd); }

};


std::array<double,3> Simulate(uint64_t strategy_id) {
  const size_t N = 90;
  const size_t t_init = N * 10'000;
  const double q = 0.9;
  const double epsilon = 0.05;
  uint64_t all_c = Strategy::AllC().ID(), all_d = Strategy::AllD().ID();
  PrivateRepGame::population_t population = {{strategy_id, N/3}, {all_c, N/3}, {all_d, N/3} };  // map of StrategyID & its size
  PrivateRepGame g(population, 1234ul);
  g.Update(t_init, q, epsilon);
  g.RestCoopCount();

  PrivateRepGame::rep_count_t rep_count;
  const size_t N_measure = N * 10'000;
  g.Update(N_measure, q, epsilon);

  auto coop_count = g.GetCoopCount();
  // IC(coop_count, rep_count);

  std::array<double,3> coop_probs;
  coop_probs[0] = static_cast<double>(coop_count.at({strategy_id,strategy_id}).at(0)) / (coop_count.at({strategy_id,strategy_id}).at(1));
  coop_probs[1] = static_cast<double>(coop_count.at({strategy_id,all_c}).at(0)) / (coop_count.at({strategy_id,all_c}).at(1));
  coop_probs[2] = static_cast<double>(coop_count.at({strategy_id,all_d}).at(0)) / (coop_count.at({strategy_id,all_d}).at(1));

  return coop_probs;
}

void CalcPrivRepCoopLevel(const std::string& fname, const std::string& outname) {
  int my_rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  std::vector<uint64_t> inputs;

  if (my_rank == 0) {
    std::cerr << "reading: " << fname << std::endl;
    std::ifstream fin(fname);
    if (!fin) {
      std::cerr << "Failed to open file " << fname << std::endl;
      throw std::runtime_error("failed to open file");
    }

    while (fin) {
      uint64_t gid;
      double c_prob, h0, h1, h2;
      fin >> gid >> c_prob >> h0 >> h1 >> h2;
      if (fin) {
        inputs.emplace_back(gid);
      }
    }
  }
  // IC(inputs);

  using output_t = std::pair<uint64_t, std::array<double, 3>>;
  std::vector<output_t> outputs(inputs.size());

  using json = nlohmann::json;
  std::function<void(caravan::Queue &)> on_init = [&inputs](caravan::Queue &q) {
    for (uint64_t str_id: inputs) {
      json buf = str_id;
      q.Push(buf);
    }
  };

  std::function<void(int64_t, const json &, const json &, caravan::Queue &)> on_result_receive = [&outputs](
    int64_t task_id, const json &input, const json &output, caravan::Queue &q) {
    output_t out = std::make_pair(input.get<uint64_t>(), std::array<double, 3>(
      {output.at(0).get<double>(), output.at(1).get<double>(), output.at(2).get<double>()}));
    outputs[task_id] = std::move(out);
  };
  std::function<json(const json &)> do_task = [](const json &input) {
    uint64_t strategy_id = input.get<uint64_t>();
    std::array<double, 3> coop_probs = Simulate(strategy_id);
    json output;
    for (double c: coop_probs) { output.emplace_back(c); }
    return output;
  };

  caravan::Start(on_init, on_result_receive, do_task, MPI_COMM_WORLD, 384, 2);

  if (my_rank == 0) {
    std::ofstream fout(outname);
    for (const output_t &o: outputs) {
      fout << o.first << ' ' << o.second[0] << ' ' << o.second[1] << ' ' << o.second[2] << "\n";
    }
    fout.close();
  }
}


int main(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  if (argc < 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file ...>" << std::endl;
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  for (int i = 1; i < argc; i++) {
    std::string outname = std::string(argv[i]) + "_priv";
    CalcPrivRepCoopLevel(argv[i], outname);
  }

  MPI_Finalize();

  return 0;
}

