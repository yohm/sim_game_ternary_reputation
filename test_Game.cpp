#include <iostream>
#include <cassert>
#include "Game.hpp"


bool Close(double d1, double d2, double tolerance = 1.0e-2) {
  return std::abs(d1-d2) < tolerance;
}

int main(int argc, char *argv[]) {
  if (argc > 1) {
    for (size_t i = 1; i < argc; i++) {
       uint64_t id = std::stoull(argv[i]);
       Game g(0.02, 0.02, id);
       std::cout << g.Inspect() << std::endl;
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
    std::cout << g.Inspect();
    assert(g.ID() == 166243799309);

    Game g2(0.02, 0.02, 166243799309);
    assert(g.rep_dynamics == g2.rep_dynamics);
    assert(g.resident_ar == g2.resident_ar);

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

    assert( g.At(Reputation::G, Reputation::B) == std::make_pair(Action::D, Reputation::N));

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
