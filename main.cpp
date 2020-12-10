#include <iostream>
#include "ReputationDynamics.hpp"

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
  std::cout << rd.Inspect() << std::endl;
  std::cout << rd.ID() << std::endl;

  ReputationDynamics rd2(rd.ID());
  assert(rd2.ID() == rd.ID());
  return 0;
}
