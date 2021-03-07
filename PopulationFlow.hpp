#include <vector>
#include <array>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "Game.hpp"

class PopulationFlow {
  public:
  // calculate the flow of the players between reputations
  // we consider a multi-graph whose nodes are {B,N,G} and edges are {XY[cd]} where X,Y are reputations.

  PopulationFlow(const Game& g) {
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
              w[Idx(X,Z,Action::C)] += h_star[i] * h_star[j] * (1.0-g.mu_e) * (1.0-g.mu_a);
            }
            else {
              w[Idx(X,Z,Action::C)] += h_star[i] * h_star[j] * (1.0-g.mu_e) * 0.5 * g.mu_a;
            }
            if (g.strategy.rd.RepAt(X,Y,Action::D) == Z) {
              w[Idx(X,Z,Action::D)] += h_star[i] * h_star[j] * g.mu_e * (1.0-g.mu_a);
            }
            else {
              w[Idx(X,Z,Action::D)] += h_star[i] * h_star[j] * g.mu_e * 0.5 * g.mu_a;
            }
          }
          else {  // act == Action::D
            if (g.strategy.rd.RepAt(X,Y,Action::D) == Z) {
              w[Idx(X,Z,Action::D)] += h_star[i] * h_star[j] * (1.0-g.mu_a);
            }
            else {
              w[Idx(X,Z,Action::D)] += h_star[i] * h_star[j] * 0.5 * g.mu_a;
            }
          }
        }
      }
    }
  }

  double Get(Reputation donor, Reputation recip, Action act) const {
    return w[ Idx(donor, recip, act) ];
  }
  const std::array<double,18>& GetAll() const { return w; }

  std::string Inspect(bool simple = true) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4);
    // calc major flow
    ss << "--- MAJOR FLOW ---" << std::endl;
    std::array< std::pair<double,size_t>, 18> flow_idx;
    for (size_t i = 0; i < 18; i++) {
      flow_idx[i] = std::make_pair(w[i], i);
    }
    std::sort(flow_idx.begin(), flow_idx.end());
    std::reverse(flow_idx.begin(), flow_idx.end());
    for (auto f_idx: flow_idx) {
      if (f_idx.first < 0.01) break;
      ss << RepString(f_idx.second) << ": " << std::setw(6) << f_idx.first << "   ";
    }
    ss << std::endl;

    if (!simple) {
      ss << "--- DETAIL ---\n";

      for (size_t i = 0; i < 18; i++) {
        ss << RepString(i) << ": " << std::setw(6) << w[i];
        ss << ((i % 6 == 5) ? "\n" : "   ");
      }
    }
    return ss.str();
  }
  std::string InspectMD() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    // calc major flow
    ss << "- Major Population Flow:" << std::endl;
    std::array<std::pair<double, size_t>, 18> flow_idx;
    for (size_t i = 0; i < 18; i++) {
      flow_idx[i] = std::make_pair(w[i], i);
    }
    std::sort(flow_idx.begin(), flow_idx.end());
    std::reverse(flow_idx.begin(), flow_idx.end());
    for (auto f_idx: flow_idx) {
      if (f_idx.first < 0.01) break;
      ss << "  - " << RepString(f_idx.second) << ": " << f_idx.first << "\n";
    }
    ss << std::endl;
    return ss.str();
  }
  std::string RepString(size_t i) const {
    std::stringstream ss;
    const auto tup = ToReps(i);
    Reputation X = std::get<0>(tup), Y = std::get<1>(tup);
    Action a = std::get<2>(tup);
    ss << "(" << X << "-" << a << "-" << Y << ")";
    return ss.str();
  }

  std::pair<double,size_t> MaxD(const PopulationFlow& other) const {
    double ans = 0.0;
    size_t idx = 0;
    for (size_t i = 0; i < 18; i++) {
      double d = std::abs(w[i] - other.w[i]);
      if (d > ans) { ans = d; idx = i; }
    }
    return std::make_pair(ans, idx);
  }

  double D1(const PopulationFlow& other) const {
    double ans = 0.0;
    for (int i = 0; i < 18; i++) {
      ans += std::abs(w[i] - other.w[i]);
    }
    return ans;
  }

  double D2(const PopulationFlow& other) const {
    double ans = 0.0;
    for (int i = 0; i < 18; i++) {
      ans += (w[i] - other.w[i]) * (w[i] - other.w[i]);
    }
    return ans;
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

