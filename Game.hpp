#ifndef GAME_HPP
#define GAME_HPP

#include <iostream>
#include <sstream>
#include "ReputationDynamics.hpp"


class Game {
  public:
  Game(double mu_e, double mu_a, const ReputationDynamics& rd, const ActionRule& ar) : mu_e(mu_e), mu_a(mu_a), rd(rd), ar(ar) {
  }
  std::string Inspect() const {
    std::stringstream ss;
    ss << "(mu_e, mu_a): (" << mu_e << ", " << mu_a << ")" << std::endl
       << "ReputationDynamics: " << std::endl << rd.Inspect()
       << "ActionRule: " << std::endl << ar.Inspect();
    return ss.str();
  }
  std::array<double,3> CalcHStarResident() {
    using v3_t = std::array<double,3>;
    v3_t ht = {1.0/3.0, 1.0/3.0, 1.0/3.0};
    const size_t N_ITER = 10000;
    double dt = 0.002;
    for (size_t t = 0; t < N_ITER; t++) {
      if (t % 100 == 99) {
        std::cerr << t << ' ' << ht[0] << ' ' << ht[1] << ' ' << ht[2] << std::endl;
      }
      v3_t k1 = HdotResident(ht);
      /*  // Euler method
      for (int i = 0; i < 3; i++) {
        ht[i] += k1[i] * dt;
      }
       */

      v3_t arg2;
      for(int i = 0; i < 3; i++) {
        k1[i] *= dt;
        arg2[i] = ht[i] + 0.5 * k1[i];
      }
      v3_t k2 = HdotResident(arg2);
      v3_t arg3;
      for(int i = 0; i < 3; i++) {
        k2[i] *= dt;
        arg3[i] = ht[i] + 0.5 * k2[i];
      }
      v3_t k3 = HdotResident(arg3);
      v3_t arg4;
      for(int i = 0; i < 3; i++) {
        k3[i] *= dt;
        arg4[i] = ht[i] + k3[i];
      }
      v3_t k4 = HdotResident(arg4);
      for(int i = 0; i < 3; i++) {
        k4[i] *= dt;
      }

      v3_t delta;
      for (int i = 0; i < 3; i++) {
        delta[i] = (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]) / 6.0;
        ht[i] += delta[i];
      }
    }
    return ht;
  }
  double mu_e, mu_a;
  ReputationDynamics rd;
  ActionRule ar;
  private:
  std::array<double,3> HdotResident(const std::array<double,3>& ht) const {
    std::array<double,3> ht_dot = {-ht[0], -ht[1], -ht[2]};
    for (int i = 0; i < 3; i++) {
      Reputation X = static_cast<Reputation>(i);
      for (int j = 0; j < 3; j++) {
        Reputation Y = static_cast<Reputation>(j);
        for (int k = 0; k < 3; k++) {
          Reputation Z = static_cast<Reputation>(k);
          int b1 = (rd.RepAt(X, Y, ar.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (rd.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
          ht_dot[k] += ht[i] * ht[j] * ( (1.0-1.5*mu_a)*((1.0-mu_e)*b1+ mu_e*b2) + 0.5*mu_a);
        }
      }
    }
    return ht_dot;
  }
};

#endif
