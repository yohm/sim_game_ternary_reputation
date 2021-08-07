#include <iostream>
#include <string>
#include <regex>
#include <cassert>
#include <icecream.hpp>
#include "Game.hpp"
#include "PopulationFlow.hpp"


bool Close(double d1, double d2, double tolerance = 1.0e-2) {
  return std::abs(d1-d2) < tolerance;
}

void PrintGame(Game& g) {
  g.ResidentEqReputation();
  std::cout << g.InspectMD();
  {
    auto a100 = g.CalcHStarFromInitialPoint({1.0, 0.0, 0.0});
    auto a010 = g.CalcHStarFromInitialPoint({0.0, 1.0, 0.0});
    auto a001 = g.CalcHStarFromInitialPoint({0.0, 0.0, 1.0});
    IC(a100, a010, a001, g.ResidentEqReputation());
  }
  IC(g.IsESS(1.01, 1.0) );
  IC(g.IsESS(1.05, 1.0) );
  IC(g.IsESS(1.1, 1.0) );
  PopulationFlow pf(g);
  std::cout << pf.InspectMD();
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::regex re1(R"(\d+)");
    std::regex re2(R"(([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]),([cd][BNG][BNG]))");
    std::cmatch m;
    for (size_t i = 1; i < argc; i++) {
      if (std::regex_match(argv[i], re1)) {
        uint64_t id = std::stoull(argv[i]);
        Game g(0.001, 0.001, id);
        PrintGame(g);
      }
      if (std::regex_match(argv[i], m, re2)) {
        ActionRule ar(0);
        ReputationDynamics rd(0);
        for (int i = 0; i < 9; i++) {
          Reputation donor = static_cast<Reputation>(i/3);
          Reputation recip = static_cast<Reputation>(i%3);
          Action act = C2A(m[i+1].str()[0]);
          Reputation rep_correct = C2R(m[i+1].str()[1]);  // reputation for the correct action
          Reputation rep_wrong = C2R(m[i+1].str()[2]);    // reputation for the wrong action
          ar.SetAction(donor, recip, act);
          rd.SetRep(donor, recip, act, rep_correct);
          rd.SetRep(donor, recip, FlipAction(act), rep_wrong);
        }
        Game g(0.02, 0.02, rd, ar);
        PrintGame(g);
      }
    }
    return 0;
  }

  {
    ReputationDynamics rd({
                            Reputation::B, Reputation::N, Reputation::G, Reputation::B, Reputation::B, Reputation::G,
                            Reputation::G, Reputation::N, Reputation::B, Reputation::G, Reputation::G, Reputation::G,
                            Reputation::N, Reputation::G, Reputation::N, Reputation::N, Reputation::N, Reputation::G
                          });
    ActionRule ar({
                   Action::C, Action::D, Action::C,
                   Action::C, Action::D, Action::D,
                   Action::D, Action::D, Action::C
                 });
    Game g(0.02, 0.02, rd, ar);
    g.ResidentEqReputation();
    std::cout << g.Inspect();
    assert(g.ID() == 166243799309);

    Game g2(0.02, 0.02, 166243799309);
    assert(g.strategy.rd == g2.strategy.rd);
    assert(g.strategy.ar == g2.strategy.ar);

    auto ht = g.ResidentEqReputation();
    assert( Close(ht[0], 0.077) );
    assert( Close(ht[1], 0.261) );
    assert( Close(ht[2], 0.661) );
    assert( Close(g.ResidentCoopProb(), 0.514 ));

    ActionRule mut(288);
    auto mut_h = g.HStarMutant(mut);
    assert( Close(mut_h[0], 0.219) );
    assert( Close(mut_h[1], 0.206) );
    assert( Close(mut_h[2], 0.573) );

    assert( g.IsESS(2.0, 1.0) == false );
    assert( Close(g.FindNegativePayoffDiff(2.0, 1.0).first, -0.199) );

    uint64_t new_rd_id = g.strategy.rd.Permute({1,0,2}).ID();
    uint64_t new_ar_id = g.strategy.ar.Permute({1,0,2}).ID();
    uint64_t new_id = (new_rd_id << 9ul) + new_ar_id;  // == 147150009650
    Game g_norm = g.NormalizedGame();
    assert(new_id == g_norm.ID() );
    auto ht_norm = g_norm.ResidentEqReputation();
    assert( Close(ht_norm[0], 0.261) );
    assert( Close(ht_norm[1], 0.077) );
    assert( Close(ht_norm[2], 0.661) );
    assert( Close(g.ResidentCoopProb(), g_norm.ResidentCoopProb()) );

    assert( g.At(Reputation::G, Reputation::B) == std::make_tuple(Action::D, Reputation::N, Reputation::G));

    auto rep_act = g.TraceReputationAndAction(Reputation::B, Reputation::G);
    assert( rep_act.first[0] == Reputation::B );
    assert( rep_act.first[1] == Reputation::G );
    assert( rep_act.first[2] == Reputation::G );
    assert( rep_act.second[0] == Action::C );
    assert( rep_act.second[1] == Action::C );
    assert( rep_act.second[2] == Action::C );

    auto rep_act2 = g.TraceReputationAndAction(Reputation::N, Reputation::G);
    assert( rep_act2.first[0] == Reputation::N );
    assert( rep_act2.first[1] == Reputation::G );
    assert( rep_act2.first[2] == Reputation::G );
    assert( rep_act2.second[0] == Action::D );
    assert( rep_act2.second[1] == Action::C );
    assert( rep_act2.second[2] == Action::C );
  }

  {
    ReputationDynamics rd(0ull);  // AllB
    ActionRule ar(511ull);  // AllC
    Game g(0.02, 0.02, rd, ar);
    auto h = g.ResidentEqReputation();
    assert(Close(h[0], 0.98));
    assert(Close(h[1], 0.01));
    assert(Close(h[2], 0.01));
    assert(Close(g.ResidentCoopProb(), 1.0));
  }

  {
    // test permutation
    ReputationDynamics rd({
                            Reputation::B, Reputation::N, Reputation::G, Reputation::B, Reputation::B, Reputation::G,
                            Reputation::G, Reputation::N, Reputation::B, Reputation::G, Reputation::G, Reputation::G,
                            Reputation::N, Reputation::G, Reputation::N, Reputation::N, Reputation::N, Reputation::B
                          });
    ActionRule ar({
                    Action::C, Action::D, Action::C,
                    Action::C, Action::D, Action::D,
                    Action::D, Action::D, Action::C
                  });

    Game g(0.02, 0.02, rd, ar);
    auto ht = g.ResidentEqReputation();
    assert( Close(ht[0], 0.27) );
    assert( Close(ht[1], 0.40) );
    assert( Close(ht[2], 0.33) );
    assert( Close(g.ResidentCoopProb(), 0.38 ));

    auto rdn_map = rd.Normalized();
    ReputationDynamics& rd2 = rdn_map.first;
    ActionRule ar2 = ar.Permute(rdn_map.second);
    Game g2(0.02, 0.02, rd2, ar2);
    std::array<int,3> map_expected = {2,1,0};
    assert( rdn_map.second == map_expected );
    auto ht2 = g2.ResidentEqReputation();
    assert( Close(ht2[2], 0.27) );
    assert( Close(ht2[1], 0.40) );
    assert( Close(ht2[0], 0.33) );
    assert( Close(g.ResidentCoopProb(), 0.38 ));
  }

  // construct leading eight
  {
    // GGC => G, GBC => *, BGC => G, BBC => *
    // GGD => B, GBD => G, BGD => B, BBD => *
    // action rule: GG => C, GB => D, BG => C, BB => **
    ReputationDynamics l1({
                            Reputation::B, Reputation::B, Reputation::B, Reputation::B, Reputation::B, Reputation::G,
                            Reputation::B, Reputation::B, Reputation::B, Reputation::B, Reputation::B, Reputation::G,
                            Reputation::G, Reputation::B, Reputation::G, Reputation::B, Reputation::B, Reputation::G
                          });
    ActionRule ar1({
                     Action::D, Action::D, Action::C,
                     Action::D, Action::D, Action::C,
                     Action::D, Action::D, Action::C
                   });
    Game g2(0.02, 0.02, l1, ar1);
    std::cout << g2.Inspect();
    assert(g2.ID() == 137863130404);
    auto res_h = g2.ResidentEqReputation();
    assert(res_h[2] > 0.95);
    assert(g2.ResidentCoopProb() > 0.95);

    assert(g2.IsESS(1.2, 1.0));
    std::pair<double,ActionRule> min_pair = g2.FindNegativePayoffDiff(1.2, 1.0);
    assert(Close(min_pair.first, 0.0001) );
    assert(Close(min_pair.second.ID(), 308) );

    auto rep_act = g2.TraceReputationAndAction(Reputation::B, Reputation::G);
    assert( rep_act.first[0] == Reputation::B );
    assert( rep_act.first[1] == Reputation::G );
    assert( rep_act.first[2] == Reputation::G );
    assert( rep_act.second[0] == Action::C );
    assert( rep_act.second[1] == Action::C );
    assert( rep_act.second[2] == Action::C );

    auto rep_act2 = g2.TraceReputationAndAction(Reputation::N, Reputation::G);
    assert( rep_act2.first[0] == Reputation::N );
    assert( rep_act2.first[1] == Reputation::G );
    assert( rep_act2.first[2] == Reputation::G );
    assert( rep_act2.second[0] == Action::C );
    assert( rep_act2.second[1] == Action::C );
    assert( rep_act2.second[2] == Action::C );
  }

  return 0;
}
