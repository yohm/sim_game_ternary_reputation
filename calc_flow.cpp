#include <iostream>
#include <vector>
#include <array>
#include <tuple>
#include <iomanip>

#include "Game.hpp"

class ReputationFlow {
  public:
  // calculate the flow of the players between reputations
  // we consider a multi-graph whose nodes are {B,N,G} and edges are {XY[cd]} where X,Y are reputations.

  ReputationFlow(Game& g) {
    for (size_t i = 0; i < w.size(); i++) { w[i] = 0.0; }

    // w_{XYc} = \sum_{Z : D_{XZc}=Y}  h_x h_z (1-e)P_{xz}
    // w_{XYd} = \sum_{Z : D_{XZd}=Y}  h_x h_z {P_{xz}*e + (1-P_{xz})}
    //         = \sum_{Z : D_{XZd}=Y}  h_x h_z {1 - (1-e)P_{xz}}

    std::array<double,3> h_star = g.ResidentEqReputation();

    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Z = static_cast<Reputation>(j);
        if (g.resident_ar.ActAt(X,Z) == Action::C) {
          const Reputation Y1 = g.rep_dynamics.RepAt(X, Z, Action::C);
          w[Idx(X,Y1,Action::C)] += h_star[i] * h_star[j] * (1.0-g.mu_e);
          const Reputation Y2 = g.rep_dynamics.RepAt(X, Z, Action::D);
          w[Idx(X,Y2,Action::D)] += h_star[i] * h_star[j] * (g.mu_e);
        }
        else {
          Reputation Y = g.rep_dynamics.RepAt(X, Z, Action::D);
          w[Idx(X,Y,Action::D)] += h_star[i] * h_star[j];
        }
      }
    }
  }

  double Get(Reputation donor, Reputation recip, Action act) const {
    return w[ Idx(donor, recip, act) ];
  }
  const std::array<double,18>& GetAll() const { return w; }

  std::string Inspect() {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4);
    for (size_t i = 0; i < 18; i++) {
      const auto tup = ToReps(i);
      Reputation X = std::get<0>(tup), Y = std::get<1>(tup);
      Action a = std::get<2>(tup);
      ss << "(" << X << "-" << a << "-" << Y << "): " << std::setw(6) << w[i];
      ss << ((i % 6 == 5) ? "\n" : "\t");
    }
    return ss.str();
  }

  private:
  std::array<double,18> w;
  size_t Idx(Reputation donor, Reputation recip, Action act) const { // return index for (X,Y,a)
    return static_cast<int>(donor) * 6 + static_cast<int>(recip) * 2 + static_cast<int>(act);
  }
  std::tuple<Reputation,Reputation,Action> ToReps(size_t i) const {
    if (i > 18) { throw std::runtime_error("invalid index"); }
    return std::make_tuple(static_cast<Reputation>(i/6), static_cast<Reputation>((i/2)%3), static_cast<Action>(i%2));
  }
};

Game& Type1() {
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
  Game g(0.02, 0.02, l1, ar1);
  return g;
}

int main(int argc, char* argv[]) {

  Game g = Type1();
  ReputationFlow rf(g);
  std::cout << g.Inspect() << rf.Inspect();

  return 0;
}