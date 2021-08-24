#include <vector>
#include <array>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Game.hpp"


class PopulationFlow {
  public:
  // calculate the detailed flow of the players between reputations
  // we consider a multi-graph whose nodes are {B,N,G} and edges represent amount of flow T_{XYa->Z}.
  // we don't take sum over Y
  // w[X,Y,act,Z]

  PopulationFlow(const Game &_g) : g(_g) {
    for (size_t i = 0; i < w.size(); i++) { w[i] = 0.0; }

    std::array<double,3> h_star = g.ResidentEqReputation();
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        Action act = g.strategy.ar.ActAt(X,Y);
        for (int k = 0; k < 3; k++) {
          Reputation Z = static_cast<Reputation>(k);
          if (act == Action::C) {
            if (g.strategy.rd.RepAt(X,Y,Action::C) == Z) {
              w[Idx(X,Y,Action::C,Z)] += h_star[i] * h_star[j] * (1.0-g.mu_e) * (1.0-g.mu_a);
            }
            else {
              w[Idx(X,Y,Action::C,Z)] += h_star[i] * h_star[j] * (1.0-g.mu_e) * 0.5 * g.mu_a;
            }
            if (g.strategy.rd.RepAt(X,Y,Action::D) == Z) {
              w[Idx(X,Y,Action::D,Z)] += h_star[i] * h_star[j] * g.mu_e * (1.0-g.mu_a);
            }
            else {
              w[Idx(X,Y,Action::D,Z)] += h_star[i] * h_star[j] * g.mu_e * 0.5 * g.mu_a;
            }
          }
          else {  // act == Action::D
            if (g.strategy.rd.RepAt(X,Y,Action::D) == Z) {
              w[Idx(X,Y,Action::D,Z)] += h_star[i] * h_star[j] * (1.0-g.mu_a);
            }
            else {
              w[Idx(X,Y,Action::D,Z)] += h_star[i] * h_star[j] * 0.5 * g.mu_a;
            }
          }
        }
      }
    }
  }
  std::string InspectMD() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4);
    // calc major flow
    double threshold = 0.0004;
    ss << "- Major Population Flow: (threshold=" << threshold << ", implementation error: *, assignment error: +)" << std::endl;
    std::array<std::pair<double, size_t>, 54> flow_idx;
    for (size_t i = 0; i < 54; i++) {
      flow_idx[i] = std::make_pair(w[i], i);
    }
    std::sort(flow_idx.begin(), flow_idx.end());
    std::reverse(flow_idx.begin(), flow_idx.end());
    for (auto f_idx: flow_idx) {
      if (f_idx.first < threshold) break;
      ss << "  - " << RepString(f_idx.second) << ": " << f_idx.first << "\n";
    }
    ss << std::endl;
    return ss.str();
  }
  private:
  const Game g;
  std::array<double,54> w;
  size_t Idx(Reputation X, Reputation Y, Action act, Reputation Z) const { // return index for (X,Y,a,Z)
    return static_cast<int>(X)*18 + static_cast<int>(Y)*6 + static_cast<int>(act)*3 + static_cast<int>(Z);
  }
  std::tuple<Reputation,Reputation,Action,Reputation> ToReps(size_t i) const {
    if (i > 54) { throw std::runtime_error("invalid index"); }
    return std::make_tuple(static_cast<Reputation>(i/18), static_cast<Reputation>((i/6)%3), static_cast<Action>((i/3)%2), static_cast<Reputation>(i%3) );
  }
  std::string RepString(size_t i) const {
    std::stringstream ss;
    const auto tup = ToReps(i);
    Reputation X = std::get<0>(tup), Y = std::get<1>(tup), Z = std::get<3>(tup);
    Action a = std::get<2>(tup);
    ss << "(" << X << Y << ":" << a << Z << ")";

    const auto no_error = g.strategy.At(X,Y);
    const Action correct_act = std::get<0>(no_error);
    const Reputation correct_rep = std::get<1>(no_error);

    if (correct_act == a && correct_rep == Z) ss << "  "; // no error occurred
    else if (correct_act == a) ss << "+ "; // only assessment error occurred
    else if (correct_act != a && std::get<2>(no_error) == Z) ss << "* ";  // only implementation error occurred
    else ss << "*+"; // both errors occurred
    return ss.str();
  }
};
