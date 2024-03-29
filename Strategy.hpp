#ifndef REPUTATION_DYNAMICS_HPP
#define REPUTATION_DYNAMICS_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <cstdint>


enum class Action {
  D = 0,  // defect
  C = 1   // cooperate
};

Action FlipAction(Action a) {
  if (a == Action::C) { return Action::D; }
  else { return Action::C; }
}

enum class Reputation {
  B = 0,   // bad
  N = 1,   // normal
  G = 2    // good
};

char A2C(Action a) {
  if (a == Action::D) { return 'd'; }
  else { return 'c'; }
}
Action C2A(char c) {
  if (c == 'd') { return Action::D; }
  else if (c == 'c') { return Action::C; }
  else { throw std::runtime_error("invalid character for action"); }
}
char R2C(Reputation r) {
  if (r == Reputation::B) { return 'B'; }
  else if (r == Reputation::N) { return 'N'; }
  else { return 'G'; }
}
Reputation C2R(char c) {
  if (c == 'B') { return Reputation::B; }
  else if (c == 'N') { return Reputation::N; }
  else if (c == 'G') { return Reputation::G; }
  else { throw std::runtime_error("invalid character for action"); }
}
std::ostream &operator<<(std::ostream &os, const Action &act) {
  os << A2C(act);
  return os;
}
std::ostream &operator<<(std::ostream &os, const Reputation &rep) {
  os << R2C(rep);
  return os;
}

class ActionRule {
  public:
  ActionRule(const std::array<Action,9>& acts) : actions(acts) {};
  ActionRule(uint64_t id) {
    if (id >= 512) { throw std::runtime_error("invalid ID for ActionRule"); }
    for (size_t i = 0; i < 9; i++) {
      if (id & (1ul << i)) { actions[i] = static_cast<Action>(1); }
      else { actions[i] = static_cast<Action>(0); }
    }
  }
  ActionRule Clone() const { return ActionRule(actions); }
  Action ActAt(const Reputation& rep_d, const Reputation& rep_r) const {
    size_t idx = 0;
    idx += static_cast<size_t>(rep_d) * 3;
    idx += static_cast<size_t>(rep_r);
    return actions[idx];
  }
  void SetAction(const Reputation& rep_d, const Reputation& rep_r, const Action& act) {
    size_t idx = 0;
    idx += static_cast<size_t>(rep_d) * 3;
    idx += static_cast<size_t>(rep_r);
    actions[idx] = act;
  }
  ActionRule Permute(std::array<int,3> map) const {
    {
      auto a = map; std::sort(a.begin(), a.end());
      assert(a[0] == 0 && a[1] == 1 && a[2] == 2);
    }
    ActionRule new_ar(0);
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      Reputation X_new = static_cast<Reputation>(map[i]);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        Reputation Y_new = static_cast<Reputation>(map[j]);
        Action A = ActAt(X, Y);
        new_ar.SetAction(X_new, Y_new, A);
      }
    }
    return new_ar;
  }

  std::string Inspect() const {
    std::stringstream ss;
    ss << "ActionRuleID: " << ID() << std::endl;
    for (size_t i = 0; i < 9; i++) {
      Reputation rep_d = static_cast<Reputation>(i / 3);
      Reputation rep_r = static_cast<Reputation>(i % 3);
      ss << "(" << rep_d << "->" << rep_r << ") : " << actions[i];
      if (i % 3 == 2) { ss << std::endl; }
      else { ss << "\t"; }
    }
    return ss.str();
  }

  uint64_t ID() const {
    int ans = 0;
    for (size_t i = 0; i < 9; i++) {
      ans += static_cast<uint64_t>(actions[i]) << i;
    }
    return ans;
  }

  bool IsSecondOrder() const {
    bool ans = true;
    for (size_t i = 0; i < 3; i++) {
      Reputation rep_r = static_cast<Reputation>(i);
      Action a = ActAt(Reputation::B, rep_r);
      Action b = ActAt(Reputation::N, rep_r);
      Action c = ActAt(Reputation::G, rep_r);
      if (a != b || b != c) {
        ans = false;
        break;
      }
    }
    return ans;
  }

  std::array<Action,9> actions;
};

bool operator==(const ActionRule& t1, const ActionRule& t2) { return t1.ID() == t2.ID(); }
bool operator!=(const ActionRule& t1, const ActionRule& t2) { return !(t1 == t2); }

class ReputationDynamics {
  public:
  ReputationDynamics(const std::array<Reputation,18>& reps) : reputations(reps) {};
  ReputationDynamics(uint64_t id) {
    // 3^18 = 387420489
    if (id >= 387420489ull) { throw std::runtime_error("invalid ID for ReputationDynamics"); }
    for (size_t i = 0; i < 18; i++) {
      reputations[i] = static_cast<Reputation>(id % 3);
      id /= 3;
    }
  }
  ReputationDynamics Clone() const { return ReputationDynamics(reputations); }
  ReputationDynamics Permute(std::array<int,3> map) const {
    {
      auto a = map; std::sort(a.begin(), a.end());
      assert(a[0] == 0 && a[1] == 1 && a[2] == 2);
    }
    ReputationDynamics new_rd(0);
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      Reputation X_new = static_cast<Reputation>(map[i]);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        Reputation Y_new = static_cast<Reputation>(map[j]);
        Reputation Z = RepAt(X, Y, Action::C);
        Reputation Z_new = static_cast<Reputation>( map[static_cast<int>(Z)] );
        new_rd.SetRep(X_new, Y_new, Action::C, Z_new);
        Z = RepAt(X, Y, Action::D);
        Z_new = static_cast<Reputation>( map[static_cast<int>(Z)] );
        new_rd.SetRep(X_new, Y_new, Action::D, Z_new);
      }
    }
    return new_rd;
  }
  std::pair<ReputationDynamics,std::array<int,3>> Normalized() const {
    using map_t = std::array<int,3>;
    std::vector<map_t> maps = { {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0} };
    size_t max = ID();
    ReputationDynamics ans = (*this);
    map_t m_max = {0,1,2};
    for (const auto& m : maps) {
      ReputationDynamics t = Permute(m);
      if (t.ID() > max) {
        max = t.ID();
        ans = t;
        m_max = m;
      }
    }
    return std::make_pair(ans, m_max);
  }
  Reputation RepAt(const Reputation& rep_d, const Reputation& rep_r, const Action& act) const {
    size_t idx = 0;
    idx += static_cast<size_t>(rep_d) * 6;
    idx += static_cast<size_t>(rep_r) * 2;
    idx += static_cast<size_t>(act);
    return reputations[idx];
  }
  void SetRep(const Reputation& rep_d, const Reputation& rep_r, const Action& act, const Reputation& new_rep) {
    size_t idx = 0;
    idx += static_cast<size_t>(rep_d) * 6;
    idx += static_cast<size_t>(rep_r) * 2;
    idx += static_cast<size_t>(act);
    reputations[idx] = new_rep;
  }

  std::string Inspect() const {
    std::stringstream ss;
    ss << "ReputationDynamicsID: " << ID() << std::endl;
    for (size_t i = 0; i < 18; i++) {
      Reputation rep_d = static_cast<Reputation>(i / 6);
      Reputation rep_r = static_cast<Reputation>((i/2) % 3);
      Action act = static_cast<Action>(i % 2);
      ss << "(" << rep_d << "->" << rep_r << "," << act << ") : " << reputations[i];
      if (i % 6 == 5) { ss << std::endl; }
      else { ss << "\t"; }
    }
    return ss.str();
  }

  uint64_t ID() const {
    int ans = 0;
    size_t i = 17;
    while(true) {
      ans *= 3;
      ans += static_cast<int>(reputations[i]);
      if (i == 0) break;
      i--;
    }
    return ans;
  }

  bool IsSecondOrder() const {
    bool ans = true;
    for (size_t i = 0; i < 6; i++) {
      Reputation rep_r = static_cast<Reputation>(i/2);
      Action act = static_cast<Action>(i % 2);
      Reputation a = RepAt(Reputation::B, rep_r, act);
      Reputation b = RepAt(Reputation::N, rep_r, act);
      Reputation c = RepAt(Reputation::G, rep_r, act);
      if (a != b || b != c) {
        ans = false;
        break;
      }
    }
    return ans;
  }

  std::array<Reputation,18> reputations;
};
bool operator==(const ReputationDynamics& t1, const ReputationDynamics& t2) { return t1.ID() == t2.ID(); }
bool operator!=(const ReputationDynamics& t1, const ReputationDynamics& t2) { return !(t1 == t2); }

// Strategy is a set of ReputationDynamics & ActionRule
class Strategy {
  public:
  Strategy(const ReputationDynamics& rep_d, const ActionRule& act_r) : rd(rep_d.ID()), ar(act_r.ID()) {};
  Strategy(uint64_t id) : rd(id>>9ull), ar(id&511ull) {};
  ReputationDynamics rd;
  ActionRule ar;
  uint64_t ID() const {
    uint64_t ar_id = ar.ID();
    uint64_t rd_id = rd.ID();
    return (rd_id << 9ull) + ar_id;
  }
  std::string Inspect() const {
    std::stringstream ss;
    ss << "StrategyID: " << ID() << std::endl
       << "(RD_id, AR_id): (" << rd.ID() << ", " << ar.ID() << ")" << std::endl
       << "--- Transitions" << std::endl;
    for (int i = 0; i < 9; i++) {
      Reputation X = static_cast<Reputation>(i/3);
      Reputation Y = static_cast<Reputation>(i%3);
      auto p = At(X, Y);
      ss << "(" << X << "->" << Y << "): " << std::get<0>(p) << std::get<1>(p) << ':' << std::get<2>(p);
      ss << ((i % 3 == 2) ? "\n" : "\t");
    }
    return ss.str();
  }
  std::tuple<Action,Reputation,Reputation> At(Reputation donor, Reputation recipient) const {
    Action a = ar.ActAt(donor, recipient);
    Reputation r = rd.RepAt(donor, recipient, a);
    Action a_not = (a == Action::C) ? Action::D : Action::C;
    Reputation r_not = rd.RepAt(donor, recipient, a_not);
    return std::make_tuple(a, r, r_not);
  }
  Action Act(Reputation donor, Reputation recipient) const { return ar.ActAt(donor, recipient); }
  Reputation Assess(Reputation donor, Reputation recipient, Action act) const { return rd.RepAt(donor, recipient, act); }
  bool IsSecondOrder() const {
    return rd.IsSecondOrder() && ar.IsSecondOrder();
  }
  static Strategy AllC() { return Strategy((387420488ull << 9ull) + 511ull); }
  static Strategy AllD() { return Strategy(0); }
};
bool operator==(const Strategy& s1, const Strategy& s2) { return s1.ID() == s2.ID(); }
bool operator!=(const Strategy& s1, const Strategy& s2) { return !(s1 == s2); }

#endif