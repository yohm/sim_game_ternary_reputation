#include <iostream>
#include "ReputationDynamics.hpp"
#include "Game.hpp"

int main() {
  std::cout << "Hello" << std::endl;

  std::cout << Reputation::G << ' ' << Action::D << std::endl;

  ActionRule p({
    Action::C, Action::D, Action::C,
    Action::C, Action::D, Action::D,
    Action::D, Action::D, Action::C
  });
  std::cout << p.Inspect() << std::endl;

  std::cout << p.ID() << std::endl;
  ActionRule p2(247);
  std::cout << p2.Inspect() << std::endl;
  assert(p2.ID() == 247);

  ReputationDynamics rd({
    Reputation::B, Reputation::N, Reputation::G, Reputation::B, Reputation::B, Reputation::G,
    Reputation::G, Reputation::N, Reputation::B, Reputation::G, Reputation::G, Reputation::G,
    Reputation::N, Reputation::G, Reputation::N, Reputation::N, Reputation::N, Reputation::G
  });
  std::cout << rd.Inspect();
  std::cout << rd.RepAt(Reputation::N, Reputation::B, Action::D) << std::endl;
  std::cout << rd.ID() << std::endl;

  ReputationDynamics rd2(rd.ID());
  assert(rd2.ID() == rd.ID());

  // test Game
  Game g(0.02, 0.02, rd, p);
  std::cout << g.Inspect();
  auto ht = g.h_star;
  std::cout << ht[0] << ' ' << ht[1] << ' ' << ht[2] << std::endl;
  std::cout << g.res_coop_prob << std::endl;

  ActionRule mut(288);
  auto mut_h = g.HStarMutant(mut);
  std::cout << mut_h[0] << ' ' << mut_h[1] << ' ' << mut_h[2] << std::endl;

  std::cout << g.IsESS(2.0, 1.0);
  return 0;
}
