#ifndef REPUTATION_DYNAMICS_HPP
#define REPUTATION_DYNAMICS_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <array>


enum class Action {
  D = 0,  // defect
  C = 1   // cooperate
};

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
  ActionRule(int id) {
    if (id >= 512 || id < 0) { throw std::runtime_error("invalid ID for ActionRule"); }
    for (size_t i = 0; i < 9; i++) {
      if (id & (1ul << i)) { actions[i] = static_cast<Action>(1); }
      else { actions[i] = static_cast<Action>(0); }
    }
  }
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

  int ID() const {
    int ans = 0;
    for (size_t i = 0; i < 9; i++) {
      ans += static_cast<int>(actions[i]) << i;
    }
    return ans;
  }

  std::array<Action,9> actions;
};

bool operator==(const ActionRule& t1, const ActionRule& t2) { return t1.ID() == t2.ID(); }
bool operator!=(const ActionRule& t1, const ActionRule& t2) { return !(t1 == t2); }

class ReputationDynamics {
  public:
  ReputationDynamics(const std::array<Reputation,18> reps) : reputations(reps) {};
  ReputationDynamics(int id) {
    // 3^18 = 387420489
    if (id >= 387420489 || id < 0) { throw std::runtime_error("invalid ID for ReputationDynamics"); }
    for (size_t i = 0; i < 18; i++) {
      reputations[i] = static_cast<Reputation>(id % 3);
      id /= 3;
    }
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

  int ID() const {
    int ans = 0;
    for (int i = 17; i >= 0; i--) {
      ans *= 3;
      ans += static_cast<int>(reputations[i]);
    }
    return ans;
  }

  std::array<Reputation,18> reputations;
};
bool operator==(const ReputationDynamics& t1, const ReputationDynamics& t2) { return t1.ID() == t2.ID(); }
bool operator!=(const ReputationDynamics& t1, const ReputationDynamics& t2) { return !(t1 == t2); }

#endif