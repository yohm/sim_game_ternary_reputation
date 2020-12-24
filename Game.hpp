#ifndef GAME_HPP
#define GAME_HPP

#include <iostream>
#include <sstream>
#include <cmath>
#include <functional>
#include "ReputationDynamics.hpp"


class Game {
  public:
  using v3d_t = std::array<double,3>;
  Game(double mu_e, double mu_a, const ReputationDynamics& rd, const ActionRule& ar) : mu_e(mu_e), mu_a(mu_a), rep_dynamics(rd), resident_ar(ar) {
    resident_h_star_ready = false;
  }
  Game(double mu_e, double mu_a, uint64_t id) : mu_e(mu_e), mu_a(mu_a), rep_dynamics(id>>9ul), resident_ar(id&511ul) {
    resident_h_star_ready = false;
  }
  std::string Inspect() const {
    std::stringstream ss;
    ss << "GameID: " << ID() << std::endl
       << "(mu_e, mu_a): (" << mu_e << ", " << mu_a << ")" << std::endl
       << "--- " << rep_dynamics.Inspect()
       << "--- " << resident_ar.Inspect();
    return ss.str();
  }
  uint64_t ID() const {
    uint64_t ar_id = resident_ar.ID();
    uint64_t rd_id = rep_dynamics.ID();
    return (rd_id << 9) + ar_id;
  }
  const double mu_e, mu_a;
  const ReputationDynamics rep_dynamics;
  const ActionRule resident_ar;
  bool IsESS(double benefit, double cost) {
    CalcHStarResident();
    double res_payoff = (benefit-cost) * resident_coop_prob;
    for (int i = 0; i < 512; i++) {
      // if (i % 100 == 0 ) { std::cerr << "checking mutant " << i << std::endl; }
      if (i == resident_ar.ID()) continue;
      ActionRule mut_ar(i);
      if (MutantPayoff(mut_ar, benefit, cost) > res_payoff) { return false; }
    }
    return true;
  }
  v3d_t ResidentEqReputation() { CalcHStarResident(); return resident_h_star; } // equilibrium reputation of resident species
  double ResidentCoopProb() { CalcHStarResident(); return resident_coop_prob; } // cooperation probability between resident species without implementation error
  v3d_t HStarMutant(const ActionRule& mutant_action_rule) {
    CalcHStarResident();
    std::function<std::array<double,3>(std::array<double,3>)> func = [this,&mutant_action_rule](std::array<double,3> x) {
      return HdotMutant(x, mutant_action_rule);
    };
    return SolveByRungeKutta(func);
  }
  double MutantPayoff(const ActionRule& mutant, double benefit, double cost) {
    CalcHStarResident();
    v3d_t mut_rep = HStarMutant(mutant);
    // cooperation probability of mutant against resident
    double mut_res_coop = CooperationProb(mutant, mut_rep, resident_h_star); // cooperation prob of mutant for resident
    double res_mut_coop = CooperationProb(resident_ar, resident_h_star, mut_rep); // cooperation prob of resident for mutant
    return benefit * res_mut_coop - cost * mut_res_coop;
  }
  std::pair<std::array<Reputation,3>,std::array<Action,3>> TraceReputationAndAction(const Reputation& init, const Reputation& resident_rep) const {
    std::array<Reputation,3> rep_hist;
    std::array<Action,3> act_hist;
    rep_hist[0] = init;
    act_hist[0] = resident_ar.ActAt(rep_hist[0], resident_rep);
    rep_hist[1] = rep_dynamics.RepAt(rep_hist[0], resident_rep, act_hist[0]);
    act_hist[1] = resident_ar.ActAt(rep_hist[1], resident_rep);
    rep_hist[2] = rep_dynamics.RepAt(rep_hist[1], resident_rep, act_hist[1]);
    act_hist[2] = resident_ar.ActAt(rep_hist[2], resident_rep);
    return std::make_pair(rep_hist, act_hist);
  }
  private:
  v3d_t resident_h_star; // equilibrium reputation of resident species
  double resident_coop_prob;  // cooperation probability of resident species
  bool resident_h_star_ready; // if true, resident_h_star and resident_coop_prob are ready
  void CalcHStarResident() {
    if (resident_h_star_ready) return;
    std::function<std::array<double,3>(std::array<double,3>)> func = [this](std::array<double,3> x) {
      return HdotResident(x);
    };
    resident_h_star = SolveByRungeKutta(func);
    resident_coop_prob = CooperationProb(resident_ar, resident_h_star, resident_h_star);
    resident_h_star_ready = true;
  }
  v3d_t HdotResident(const std::array<double,3>& ht) const {
    v3d_t ht_dot = {-ht[0], -ht[1], -ht[2]};
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        for (int k = 0; k < 3; k++) {
          Reputation Z = static_cast<Reputation>(k);
          int b1 = (rep_dynamics.RepAt(X, Y, resident_ar.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (rep_dynamics.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
          ht_dot[k] += ht[i] * ht[j] * ( (1.0-1.5*mu_a)*((1.0-mu_e)*b1+ mu_e*b2) + 0.5*mu_a);
        }
      }
    }
    return ht_dot;
  }
  v3d_t HdotMutant(const std::array<double,3>& ht, const ActionRule& mutant_action_rule) const {
    v3d_t ht_dot = {-ht[0], -ht[1], -ht[2]};
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        for (int k = 0; k < 3; k++) {
          Reputation Z = static_cast<Reputation>(k);
          int b1 = (rep_dynamics.RepAt(X, Y, mutant_action_rule.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (rep_dynamics.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
          ht_dot[k] += ht[i] * resident_h_star[j] * ((1.0 - 1.5 * mu_a) * ((1.0 - mu_e) * b1 + mu_e * b2) + 0.5 * mu_a);
        }
      }
    }
    return ht_dot;
  }
  v3d_t SolveByRungeKutta(std::function<v3d_t (v3d_t)>& func) const {
    v3d_t ht = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
    const size_t N_ITER = 1'000'000;
    double dt = 0.01;
    const double conv_tolerance = 1.0e-6 * dt;
    for (size_t t = 0; t < N_ITER; t++) {
#ifdef DEBUG
      if (t % 10000 == 9999) {
        std::cerr << t << ' ' << ht[0] << ' ' << ht[1] << ' ' << ht[2] << std::endl;
      }
#endif
      v3d_t k1 = func(ht);
      v3d_t arg2;
      for(int i = 0; i < 3; i++) {
        k1[i] *= dt;
        arg2[i] = ht[i] + 0.5 * k1[i];
      }
      v3d_t k2 = func(arg2);
      v3d_t arg3;
      for(int i = 0; i < 3; i++) {
        k2[i] *= dt;
        arg3[i] = ht[i] + 0.5 * k2[i];
      }
      v3d_t k3 = func(arg3);
      v3d_t arg4;
      for(int i = 0; i < 3; i++) {
        k3[i] *= dt;
        arg4[i] = ht[i] + k3[i];
      }
      v3d_t k4 = func(arg4);
      for(int i = 0; i < 3; i++) {
        k4[i] *= dt;
      }
      v3d_t delta;
      double sum = 0.0;
      for (int i = 0; i < 3; i++) {
        delta[i] = (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]) / 6.0;
        ht[i] += delta[i];
        sum += ht[i];
      }
      // normalize ht
      double sum_inv = 1.0 / sum;
      for (int i = 0; i < 3; i++) { ht[i] *= sum_inv; }

      if (std::abs(delta[0]) < conv_tolerance &&
          std::abs(delta[1]) < conv_tolerance &&
          std::abs(delta[2]) < conv_tolerance) {
        break;
      }
    }
    return ht;
  }
  double CooperationProb(const ActionRule& donor_action, const std::array<double,3>& donor_reputation, const std::array<double,3>& recip_reputation) const {
    double sum = 0.0;
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        if (donor_action.ActAt(X, Y) == Action::C) {
          sum += donor_reputation[i] * recip_reputation[j];
        }
      }
    }
    return sum;
  }
};

#endif
