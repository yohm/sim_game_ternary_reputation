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

  {
    // when GG=>C is fixed, there are 255 action rules
    for (size_t i = 0; i < 256; i++) {
      ActionRule ar(i + 256);
      assert(ar.ActAt(Reputation::G, Reputation::G) == Action::C);
    }
  }

  {
    // when GGC => G and GGD => B are fixed, there are 3^16 = 43046721 types of reputation dynamics
    // add 2*3^17 + 0*3^16 = 258280326
    uint64_t fixed = 258280326ull;
    for (size_t i = 0; i < 43046721; i+=100) {
      if (i % 10000000 == 0) { std::cerr << i << std::endl; }
      ReputationDynamics rd(fixed + i);
      assert(rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G);
      assert(rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B);
    }

    {
      // when GGC => G and GGD => B are fixed, there are 3^16 = 43046721 types of reputation dynamics
      // add 2*3^17 + 0*3^16 = 258280326
      uint64_t fixed = 258280326ull;
      uint64_t i = 21046724;
      ReputationDynamics rd(fixed + i);
      ActionRule base(511);  // use AllC as a baseline
      std::vector< std::pair<Reputation,Reputation> > free_pairs;
      for (int i = 0; i < 3; i++) {
        Reputation X = static_cast<Reputation>(i);
        for (int j = 0; j < 3; j++) {
          Reputation Y = static_cast<Reputation>(j);
          if (X == Reputation::G && Y == Reputation::G) continue;
          if (rd.RepAt(X, Y, Action::C) == rd.RepAt(X, Y, Action::D)) {
            base.SetAction(X,Y, Action::D);
          }
          else {
            free_pairs.emplace_back( std::make_pair(X,Y) );
          }
        }
      }
      std::cerr << rd.Inspect();
      std::cerr << base.Inspect();
      for (auto p : free_pairs) {
        std::cerr << p.first << ' ' << p.second << std::endl;
      }
    }
  }

  return 0;
}
