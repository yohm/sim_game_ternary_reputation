#include <iostream>
#include <cassert>
#include <icecream.hpp>
#include "PrivateRepGame.hpp"
#include "icecream.hpp"


bool Close(double d1, double d2, double tolerance = 1.0e-2) {
  return std::abs(d1-d2) < tolerance;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    for (size_t i = 1; i < argc; i++) {
      uint64_t strategy_id = std::stoull(argv[i]);
      size_t N = 90;
      double q = 0.9, epsilon = 0.05;
      uint64_t all_c = Strategy::AllC().ID(), all_d = Strategy::AllD().ID();
      PrivateRepGame::population_t population = {{strategy_id, N/3}, {all_c, N/3}, {all_d, N/3} };
      PrivateRepGame g(population, 123456789ull);

      size_t t_init = 1'000;
      g.Update(N * t_init, q, epsilon);
      g.ResetCoopCount();
      std::cerr << "initialization finished\n";

      size_t t_measure = 1'000;
      g.Update(N * t_measure, q, epsilon);

      auto coop_count = g.GetCoopCount();
      // IC(coop_count, rep_count);

      g.PrintM();

      std::array<double,3> coop_probs;
      coop_probs[0] = static_cast<double>(coop_count.at({strategy_id,strategy_id}).at(0)) / (coop_count.at({strategy_id,strategy_id}).at(1));
      coop_probs[1] = static_cast<double>(coop_count.at({strategy_id,all_c}).at(0)) / (coop_count.at({strategy_id,all_c}).at(1));
      coop_probs[2] = static_cast<double>(coop_count.at({strategy_id,all_d}).at(0)) / (coop_count.at({strategy_id,all_d}).at(1));
      IC(coop_count);
    }
    return 0;
  }


  return 0;
}
