#include <iostream>
#include <cassert>
#include "ReputationDynamics.hpp"

int main(int argc, char *argv[]) {

  { // testing output
    std::cout << Action::C << ' ' << Action::D << std::endl;
    std::cout << Reputation::B << ' ' << Reputation::N << ' ' << Reputation::G << std::endl;

    assert(static_cast<Action>(0) == Action::D );
    assert(static_cast<Action>(1) == Action::C );
    assert(static_cast<int>(Action::D) == 0);
    assert(static_cast<int>(Action::C) == 1);

    assert(static_cast<Reputation>(0) == Reputation::B );
    assert(static_cast<Reputation>(1) == Reputation::N );
    assert(static_cast<Reputation>(2) == Reputation::G );
    assert(static_cast<int>(Reputation::B) == 0);
    assert(static_cast<int>(Reputation::N) == 1);
    assert(static_cast<int>(Reputation::G) == 2);
  }

  { // testing action rule
    ActionRule p({
                   Action::C, Action::D, Action::C,
                   Action::C, Action::D, Action::D,
                   Action::D, Action::D, Action::C
                 });
    std::cout << p.Inspect();

    assert(p.ID() == 269);
    assert(p.ActAt(Reputation::B, Reputation::N) == Action::D);
    assert(p.ActAt(Reputation::N, Reputation::G) == Action::D);
    assert(p.ActAt(Reputation::G, Reputation::G) == Action::C);

    ActionRule p2( 0b100001101 );
    assert(p == p2);

    p2.SetAction( Reputation::B, Reputation::N, Action::C );
    assert(p != p2);

  }

  {
    // testing reputation dynamics
    ReputationDynamics rd({
                            Reputation::B, Reputation::N, Reputation::G, Reputation::B, Reputation::B, Reputation::G,
                            Reputation::G, Reputation::N, Reputation::B, Reputation::G, Reputation::G, Reputation::G,
                            Reputation::N, Reputation::G, Reputation::N, Reputation::N, Reputation::N, Reputation::G
                          });
    std::cout << rd.Inspect();

    assert( rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G);
    assert( rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::N);
    assert( rd.ID() == 324694920 );
    std::cout << rd.ID() << std::endl;

    ReputationDynamics rd2( rd.ID() );
    assert( rd == rd2 );

    rd2.SetRep( Reputation::G, Reputation::G, Action::C, Reputation::B );
    assert( rd2.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::B );
    assert( rd != rd2 );
  }

  return 0;
}
