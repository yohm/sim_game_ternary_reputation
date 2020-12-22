#include <iostream>
#include <cassert>
#include "Game.hpp"


bool Close(double d1, double d2, double tolerance = 1.0e-2) {
  return std::abs(d1-d2) < tolerance;
}

int main(int argc, char *argv[]) {
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
    assert(g2.IsESS(2.0, 1.0));

    assert(g2.IsESS(1.2, 1.0));
  }

  return 0;
}
