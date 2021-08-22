#ifndef GAME_HPP
#define GAME_HPP

#include <iostream>
#include <sstream>
#include <cmath>
#include <functional>
#include <iomanip>
#include <icecream.hpp>
#include <Eigen/Dense>
#include "Strategy.hpp"

class Game {
  public:
  using v3d_t = std::array<double,3>;
  Game(double mu_e, double mu_a, const ReputationDynamics& rd, const ActionRule& ar) : mu_e(mu_e), mu_a(mu_a), strategy(rd, ar) {
    resident_h_star_ready = false;
  }
  Game(double mu_e, double mu_a, uint64_t id) : mu_e(mu_e), mu_a(mu_a), strategy(id) {
    resident_h_star_ready = false;
  }
  Game(double mu_e, double mu_a, uint64_t id, double coop_prob, const std::array<double,3>& h_star) : mu_e(mu_e), mu_a(mu_a), strategy(id) {
    resident_coop_prob = coop_prob;
    resident_h_star = h_star;
    resident_h_star_ready = true;
  }
  std::string Inspect() const {
    std::stringstream ss;
    const auto& h = resident_h_star;
    ss << "GameID: " << ID() << std::endl
       << "(mu_e, mu_a): (" << mu_e << ", " << mu_a << ")" << std::endl
       << "(RD_id, AR_id): (" << strategy.rd.ID() << ", " << strategy.ar.ID() << ")" << std::endl
       << "--- Transition of residents" << std::endl;
    for (int i = 0; i < 9; i++) {
      Reputation X = static_cast<Reputation>(i/3);
      Reputation Y = static_cast<Reputation>(i%3);
      auto p = At(X, Y);
      ss << "(" << X << "->" << Y << "): " << std::get<0>(p) << std::get<1>(p) << ':' << std::get<2>(p);
      ss << ((i % 3 == 2) ? "\n" : "\t");
    }
    if (resident_h_star_ready) {
      ss << "(c_prob,h0,h1,h2): " << ResidentCoopProb() << ' ' << h[0] << ' ' << h[1] << ' ' << h[2] << std::endl;
    }
    return ss.str();
  }
  std::string Inspect() {
    ResidentEqReputation();
    const Game* pg = this;
    return pg->Inspect();
  }
  std::string InspectMD() const {
    std::stringstream ss;
    ss << "- GameID: " << ID() << "\n"
       << "  - RD_id, AR_id: " << strategy.rd.ID() << ", " << strategy.ar.ID() << "\n"
       << "  - mu_a, mu_e: " << mu_a << ' ' << mu_e << "\n"
       << "- Prescriptions:\n\n"
       << "  | | | |\n"
       << "  |-|-|-|\n";
    for (int i = 0; i < 9; i++) {
      Reputation X = static_cast<Reputation>(i/3);
      Reputation Y = static_cast<Reputation>(i%3);
      auto p = At(X, Y);
      ss << "  | (" << X << "->" << Y << ") " << std::get<0>(p) << std::get<1>(p) << ':' << std::get<2>(p) << " ";
      ss << ((i % 3 == 2) ? " |\n" : "");
    }
    ss << "\n" << std::fixed << std::setprecision(3);
    ss << "- Cooperation Probability: " << ResidentCoopProb() << "\n"
       << "- Population:" << "\n"
       << "  - h_B: " << resident_h_star[0] << "\n"
       << "  - h_N: " << resident_h_star[1] << "\n"
       << "  - h_G: " << resident_h_star[2] << "\n";
    Game g2(1.0e-3, 1.0e-3, ID());
    ss << "- Cooperation Probability (mu=1e-3): " << g2.ResidentCoopProb() << "\n"
       << "- Population (mu=1e-3):" << "\n"
       << "  - h_B: " << g2.resident_h_star[0] << "\n"
       << "  - h_N: " << g2.resident_h_star[1] << "\n"
       << "  - h_G: " << g2.resident_h_star[2] << "\n";

    auto cont_payoff = ContinuationPayoff(0.5, 2.0, 1.0, mu_e);
    ss << "- Continuation payoff (w=0.5, b/c=2.0):" << "\n"
       << "  - v_B: " << cont_payoff[0] << "\n"
       << "  - v_N: " << cont_payoff[1] << "\n"
       << "  - v_G: " << cont_payoff[2] << "\n";
    return ss.str();
  }
  uint64_t ID() const {
    return strategy.ID();
  }
  const double mu_e, mu_a;
  const Strategy strategy;
  std::tuple<Action,Reputation,Reputation> At(Reputation donor, Reputation recipient) const {
    return strategy.At(donor, recipient);
  }
  Game NormalizedGame() const {
    auto h = ResidentEqReputation();

    int Gi = 0;
    if (h[0] >= h[1] && h[0] >= h[2]) { Gi = 0; }
    else if (h[1] >= h[0] && h[1] >= h[2]) { Gi = 1; }
    else if (h[2] >= h[0] && h[2] >= h[1]) { Gi = 2; }
    else { throw std::runtime_error("must not happen"); }

    Reputation Gn = static_cast<Reputation>(Gi);
    Reputation Bn = strategy.rd.RepAt(Gn, Gn, Action::D);
    if (Gn == Bn) { throw std::runtime_error("must not happen"); }
    int Bi = static_cast<int>(Bn);
    int Ni = 3 - Bi - Gi;

    std::array<int,3> map = {0,0,0};
    map[Gi] = 2;
    map[Ni] = 1;
    map[Bi] = 0;

    ReputationDynamics new_repd = strategy.rd.Permute(map);
    ActionRule new_ar = strategy.ar.Permute(map);
    uint64_t ar_id = new_ar.ID();
    uint64_t rd_id = new_repd.ID();
    uint64_t new_g_id = (rd_id << 9) + ar_id;

    std::array<double,3> new_h = {h[Bi], h[Ni], h[Gi]};

    return Game(mu_e, mu_a, new_g_id, ResidentCoopProb(), new_h);
  }
  bool IsESS(double benefit, double cost) const {
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    auto p = FindNegativePayoffDiff(benefit, cost);
    // IC(p.first, p.second.ID());
    return p.first > 0.0;
  }
  std::pair<double,ActionRule> FindNegativePayoffDiff(double benefit, double cost) const { // an index of evolutionary stability
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    double res_payoff = MutantPayoff(strategy.ar, benefit, cost);
    double min = std::numeric_limits<double>::max();
    ActionRule highest_mut = strategy.ar;
    for (int i = 0; i < 512; i++) {
      if (i == strategy.ar.ID()) continue;
      ActionRule mut_ar(i);
      double d = res_payoff - MutantPayoff(mut_ar, benefit, cost);
      if (d < min) { min = d; highest_mut = mut_ar; }
      if (min < 0.0) break;
    }
    return std::make_pair(min, highest_mut);
  }
  std::array<double,2> ESS_Benefit_Range(double lower_bound_max = std::numeric_limits<double>::max(), double upper_bound_min = std::numeric_limits<double>::min()) const {
    // stop calculating b_range when lower_bound exceeds lower_bound_max
    // or upper_bound go below upper_bound_min
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    double res_res_coop = ResidentCoopProb();
    // IC(res_res_coop, MutantCoopProbs(strategy.ar));

    double b_lower_bound = std::numeric_limits<double>::min();
    double b_upper_bound = std::numeric_limits<double>::max();

    for (int i = 0; i < 512; i++) {
      if (i == strategy.ar.ID()) continue;
      ActionRule mut_ar(i);
      auto p = MutantCoopProbs(mut_ar);
      double mut_res_coop = p.first;
      double res_mut_coop = p.second;

      if (res_res_coop > res_mut_coop) {
        double b_lower = (res_res_coop - mut_res_coop) / (res_res_coop - res_mut_coop);
        if (b_lower > b_lower_bound) { b_lower_bound = b_lower; }
      }
      else if (res_res_coop < res_mut_coop) {
        double b_upper = (res_res_coop - mut_res_coop) / (res_res_coop - res_mut_coop);
        if (b_upper < b_upper_bound) { b_upper_bound = b_upper; }  // update the upper bound of b
      }
      else {  // res_coop_prob == mut_res_coop
        if (mut_res_coop <= res_res_coop) {  // cannot be ESS
          b_lower_bound = std::numeric_limits<double>::max();
          b_upper_bound = std::numeric_limits<double>::min();
        }
      }

      if (b_lower_bound > b_upper_bound) break;
      if (b_lower_bound <= lower_bound_max || b_upper_bound >= upper_bound_min) break;
    }

    return std::array<double,2>({b_lower_bound, b_upper_bound});
  }
  v3d_t ResidentEqReputation() { CalcHStarResident(); return resident_h_star; } // equilibrium reputation of resident species
  v3d_t ResidentEqReputation() const {
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    return resident_h_star;
  }
  double ResidentCoopProb() { CalcHStarResident(); return resident_coop_prob; } // cooperation probability between resident species without implementation error
  double ResidentCoopProb() const {
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    return resident_coop_prob;
  }
  v3d_t HStarMutant(const ActionRule& mutant_action_rule) const {
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    std::function<std::array<double,3>(std::array<double,3>)> func = [this,&mutant_action_rule](std::array<double,3> x) {
      return HdotMutant(x, mutant_action_rule);
    };
    return SolveByRungeKutta(func);
  }
  std::pair<double,double> MutantCoopProbs(const ActionRule& mutant) const {
    if (!resident_h_star_ready) throw std::runtime_error("cache is not ready");
    v3d_t mut_rep = HStarMutant(mutant);
    // cooperation probability of mutant against resident
    double mut_res_coop = CooperationProb(mutant, mut_rep, resident_h_star); // cooperation prob of mutant for resident
    double res_mut_coop = CooperationProb(strategy.ar, resident_h_star, mut_rep); // cooperation prob of resident for mutant
    return std::make_pair(mut_res_coop, res_mut_coop);
  }
  double MutantPayoff(const ActionRule& mutant, double benefit, double cost) const {
    auto p = MutantCoopProbs(mutant);
    double mut_res_coop = p.first;
    double res_mut_coop = p.second;
    return benefit * res_mut_coop - cost * mut_res_coop;
  }
  std::pair<std::array<Reputation,3>,std::array<Action,3>> TraceReputationAndAction(const Reputation& init, const Reputation& resident_rep) const {
    std::array<Reputation,3> rep_hist;
    std::array<Action,3> act_hist;
    rep_hist[0] = init;
    act_hist[0] = strategy.ar.ActAt(rep_hist[0], resident_rep);
    rep_hist[1] = strategy.rd.RepAt(rep_hist[0], resident_rep, act_hist[0]);
    act_hist[1] = strategy.ar.ActAt(rep_hist[1], resident_rep);
    rep_hist[2] = strategy.rd.RepAt(rep_hist[1], resident_rep, act_hist[1]);
    act_hist[2] = strategy.ar.ActAt(rep_hist[2], resident_rep);
    return std::make_pair(rep_hist, act_hist);
  }
  std::array<double,3> ContinuationPayoff(double w, double benefit, double cost, double mu_e) const {  // calculate continuation payoff for each reputation taking into account implementation error
    auto h = ResidentEqReputation();
    Eigen::Matrix3d A;
    A << 0,0,0, 0,0,0, 0,0,0;
    for (int x = 0; x < 3; x++) {
      Reputation X = static_cast<Reputation>(x);
      A(x, x) += 1.0;
      for (int y = 0; y < 3; y++) {
        Reputation Y = static_cast<Reputation>(y);
        auto next = At(X, Y);
        Reputation Z = std::get<1>(next);
        int z = static_cast<int>(Z);
        A(x, z) -= w * h[y] * (1.0 - mu_e);
        Reputation Z_not = std::get<2>(next);
        int z_not = static_cast<int>(Z_not);
        A(x, z_not) -= w * h[y] * mu_e;
      }
    }

    Eigen::Vector3d V;
    V << 0,0,0;
    for (int x = 0; x < 3; x++) {
      Reputation X = static_cast<Reputation>(x);
      for (int y = 0; y < 3; y++) {
        Reputation Y = static_cast<Reputation>(y);
        if (strategy.ar.ActAt(Y, X) == Action::C) { V(x) += benefit * h[y] * (1.0-mu_e); }
        if (strategy.ar.ActAt(X, Y) == Action::C) { V(x) -= cost * h[y] * (1.0-mu_e); }
      }
    }

    Eigen::Vector3d ans_e = A.colPivHouseholderQr().solve(V);
    ans_e *= (1.0 - w);  // normalize
    return {ans_e(0), ans_e(1), ans_e(2)};
  }
  v3d_t CalcHStarFromInitialPoint(const v3d_t & init) {
    std::function<std::array<double,3>(std::array<double,3>)> func = [this](std::array<double,3> x) {
      return HdotResident(x);
    };
    auto ans = SolveByRungeKutta(func, init);
    return ans;
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
    resident_coop_prob = CooperationProb(strategy.ar, resident_h_star, resident_h_star);
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
          int b1 = (strategy.rd.RepAt(X, Y, strategy.ar.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (strategy.rd.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
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
          int b1 = (strategy.rd.RepAt(X, Y, mutant_action_rule.ActAt(X, Y)) == Z) ? 1 : 0;
          int b2 = (strategy.rd.RepAt(X, Y, Action::D) == Z) ? 1 : 0;
          ht_dot[k] += ht[i] * resident_h_star[j] * ((1.0 - 1.5 * mu_a) * ((1.0 - mu_e) * b1 + mu_e * b2) + 0.5 * mu_a);
        }
      }
    }
    return ht_dot;
  }
  v3d_t SolveByRungeKutta(std::function<v3d_t (v3d_t)>& func, const v3d_t& init = {1.0/3.0,1.0/3.0,1.0/3.0}) const {
    v3d_t ht = init;
    const size_t N_ITER = 10'000'000;
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
      if (t == N_ITER-1) {
        IC(Inspect(), delta, ht);
        throw std::runtime_error("does not converge");
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
