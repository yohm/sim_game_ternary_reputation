#ifndef GAME_HPP
#define GAME_HPP

#include <iostream>
#include <sstream>
#include <cmath>
#include "ReputationDynamics.hpp"


class Game {
  public:
  Game(double mu_e, double mu_a, const ReputationDynamics& rd, const ActionRule& ar) : mu_e(mu_e), mu_a(mu_a), rd(rd), res_ar(ar) {
    h_star = CalcHStarResident();
    res_coop_prob = CooperationProb(res_ar, h_star);
  }
  std::string Inspect() const {
    std::stringstream ss;
    ss << "(mu_e, mu_a): (" << mu_e << ", " << mu_a << ")" << std::endl
       << "ReputationDynamics: " << std::endl << rd.Inspect()
       << "ActionRule: " << std::endl << res_ar.Inspect();
    return ss.str();
  }
  double mu_e, mu_a;
  ReputationDynamics rd;
  ActionRule res_ar;
  std::array<double,3> h_star; // equilibrium reputation of residnet species
  double res_coop_prob;
  std::array<double,3> HStarMutant(const ActionRule& mutant_action_rule) const {
    std::function<std::array<double,3>(std::array<double,3>)> func = [this,&mutant_action_rule](std::array<double,3> x) {
      return HdotMutant(x, mutant_action_rule);
    };
    return SolveByRungeKutta(func);
  }
  private:
  std::array<double,3> CalcHStarResident() const {
    std::function<std::array<double,3>(std::array<double,3>)> func = [this](std::array<double,3> x) {
      return HdotResident(x);
    };
    return SolveByRungeKutta(func);
  }
  std::array<double,3> HdotResident(const std::array<double,3>& ht) const {
    std::array<double,3> ht_dot = {-ht[0], -ht[1], -ht[2]};
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        for (int k = 0; k < 3; k++) {
          Reputation Z = static_cast<Reputation>(k);
          int b1 = (rd.RepAt(X, Y, res_ar.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (rd.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
          ht_dot[k] += ht[i] * ht[j] * ( (1.0-1.5*mu_a)*((1.0-mu_e)*b1+ mu_e*b2) + 0.5*mu_a);
        }
      }
    }
    return ht_dot;
  }
  std::array<double,3> HdotMutant(const std::array<double,3>& ht, const ActionRule& mutant_action_rule) const {
    std::array<double,3> ht_dot = {-ht[0], -ht[1], -ht[2]};
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        for (int k = 0; k < 3; k++) {
          Reputation Z = static_cast<Reputation>(k);
          int b1 = (rd.RepAt(X, Y, mutant_action_rule.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (rd.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
          ht_dot[k] += ht[i] * h_star[j] * ( (1.0-1.5*mu_a)*((1.0-mu_e)*b1+ mu_e*b2) + 0.5*mu_a);
        }
      }
    }
    return ht_dot;
  }
  std::array<double,3> SolveByRungeKutta(std::function<std::array<double,3>(std::array<double,3>)>& func) const {
    using v3_t = std::array<double,3>;
    v3_t ht = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
    const size_t N_ITER = 10000;
    double dt = 0.002;
    const double conv_tolerance = 1.0e-4 * dt;
    for (size_t t = 0; t < N_ITER; t++) {
      if (t % 100 == 99) {
        std::cerr << t << ' ' << ht[0] << ' ' << ht[1] << ' ' << ht[2] << std::endl;
      }
      v3_t k1 = func(ht);
      v3_t arg2;
      for(int i = 0; i < 3; i++) {
        k1[i] *= dt;
        arg2[i] = ht[i] + 0.5 * k1[i];
      }
      v3_t k2 = func(arg2);
      v3_t arg3;
      for(int i = 0; i < 3; i++) {
        k2[i] *= dt;
        arg3[i] = ht[i] + 0.5 * k2[i];
      }
      v3_t k3 = func(arg3);
      v3_t arg4;
      for(int i = 0; i < 3; i++) {
        k3[i] *= dt;
        arg4[i] = ht[i] + k3[i];
      }
      v3_t k4 = func(arg4);
      for(int i = 0; i < 3; i++) {
        k4[i] *= dt;
      }
      v3_t delta;
      for (int i = 0; i < 3; i++) {
        delta[i] = (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]) / 6.0;
        ht[i] += delta[i];
      }
      if (std::abs(delta[0]) < conv_tolerance &&
          std::abs(delta[1]) < conv_tolerance &&
          std::abs(delta[2]) < conv_tolerance) {
        break;
      }
    }
    return ht;
  }
  double CooperationProb(const ActionRule& mut_ar, const std::array<double,3>& mut_h_star) {
    double sum = 0.0;
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        if (mut_ar.ActAt(X, Y) == Action::C) {
          sum += mut_h_star[i] * h_star[j];
        }
      }
    }
    return sum;
  }
};

#endif
