#include <iostream>
#include <fstream>
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
      ss << RepString(i)  << ": " << std::setw(6) << w[i];
      ss << ((i % 6 == 5) ? "\n" : "\t");
    }
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

  std::pair<double,size_t> MaxD(const ReputationFlow& other) const {
    double ans = 0.0;
    size_t idx = 0;
    for (size_t i = 0; i < 18; i++) {
      double d = std::abs(w[i] - other.w[i]);
      if (d > ans) { ans = d; idx = i; }
    }
    return std::make_pair(ans, idx);
  }

  double D1(const ReputationFlow& other) const {
    double ans = 0.0;
    for (int i = 0; i < 18; i++) {
      ans += std::abs(w[i] - other.w[i]);
    }
    return ans;
  }

  double D2(const ReputationFlow& other) const {
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

ReputationFlow GetRepFlow(uint64_t gid) {
  const double mu_e = 0.02, mu_a = 0.02;
  Game g(mu_e, mu_a, gid);
  return ReputationFlow(g);
}

void test_ReputationFlow() {
  {
    Game g(0.02, 0.02, 137863130404);  // leading-eight like strategy
    ReputationFlow rf(g);
    std::cout << g.Inspect() << rf.Inspect();
  }

  {
    Game g(0.02, 0.02, 140323977652);  // type 13
    ReputationFlow rf(g);
    std::cout << g.Inspect() << rf.Inspect();
  }

  {
    ReputationFlow rf1 = GetRepFlow(137863130404), rf2 = GetRepFlow(140323977652);
    std::cout << "D: " << rf1.D1(rf2) << ' ' << std::sqrt(rf1.D2(rf2)) << std::endl;
  }

  {
    std::vector<uint64_t> gids = {140322844084, 141139137460, 140325083572, 141140271028, 141956564404, 73671208375};
    ReputationFlow rf = GetRepFlow(140323977652);
    for (auto gid: gids) {
      ReputationFlow other = GetRepFlow(gid);
      std::cout << "D: " << rf.D1(other) << ' ' << std::sqrt(rf.D2(other)) << std::endl;
    }
  }
}

void CalcDs(uint64_t gid, const std::string& fname) {
  ReputationFlow rf = GetRepFlow(gid);

  std::ifstream fin(fname);
  std::vector<uint64_t> inputs;

  while(fin) {
    uint64_t org_gid,gid2;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid2 >> c_prob >> h0 >> h1 >> h2;
    ReputationFlow other = GetRepFlow(gid2);
    auto d_pair = rf.MaxD(other);
    std::cout << gid2 << ' ' << d_pair.first << ' ' << rf.RepString(d_pair.second) << ' ' << rf.D1(other) << ' ' << std::sqrt(rf.D2(other)) << std::endl;
    if (d_pair.first > 0.04) {
      Game g(0.02, 0.02, gid), g2(0.02, 0.02, gid2);
      std::cout << g.Inspect() << g2.Inspect();
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  // test_ReputationFlow();

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <base_gid> <fname>" << std::endl;
    throw std::runtime_error("invalid usage");
  }
  CalcDs(std::stoull(argv[1]), argv[2]);

  return 0;
}