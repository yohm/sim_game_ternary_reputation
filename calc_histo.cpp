#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>
#include <regex>
#include <omp.h>

#include "ReputationDynamics.hpp"
#include "Game.hpp"

class HistoNormalBin {
  public:
  HistoNormalBin(double bin_size) : bin(bin_size) {};
  void Add(double v) {
    int key = val_to_binidx(v);
    if( histo.find(key) == histo.end() ) { histo[key] = 0; }
    histo[key]++;
  }
  std::map<double,double> Frequency() const {
    std::map<double,double> result;
    if (histo.empty()) { return result;}
    int key_min = histo.begin()->first;
    int key_max = histo.rbegin()->first;
    for( int i = key_min; i <= key_max; i++ ) {
      double val = binidx_to_val( i );
      size_t n = ( histo.find(i) == histo.end() ) ? 0 : histo.at(i);
      double freq = n / binidx_to_binsize( i );
      result[val] = freq;
    }
    return result;
  }
  void Merge(const HistoNormalBin& other) {
    if (bin != other.bin) { throw std::runtime_error("bin size must be same"); }
    for (auto kv : other.histo) {
      if (histo.find(kv.first) == histo.end()) {
        histo[kv.first] = kv.second;
      }
      else {
        histo[kv.first] += kv.second;
      }
    }
  }
  const double bin;
  private:
  int val_to_binidx(double v) const {
    return static_cast<int>( std::floor(v/bin) );
  };
  double binidx_to_val(int i) const {
    return i * bin;
  };
  double binidx_to_binsize(int i) const {
    return bin;
  };
  std::map<int, size_t> histo;
};


// return unmatched pattern
std::string Match(const Game& g, const std::vector<std::string>& patterns) {
  const auto c2r = [](char c)->Reputation {
    if (c == 'B') { return Reputation::B; }
    else if (c == 'N') { return Reputation::N; }
    else if (c == 'G') { return Reputation::G; }
    else { throw std::runtime_error("unknown reputation"); }
  };
  const auto c2a = [](char c)->Action {
    if (c == 'c') { return Action::C; }
    else if (c == 'd') { return Action::D; }
    else { throw std::runtime_error("unknown action"); }
  };
  const auto IsIn = [&c2r](Reputation rep, const std::string& set_str)->bool {
    for (char c: set_str) {
      if (rep == c2r(c)) { return true; }
    }
    return false;
  };
  // examples:
  // 1. GG:cG, GG:c*, GB:*B
  // 2. GGd:B
  // 3. GB:c[NG], GB:*[NG]
  // 4. BGd:[BN]
  // 5. GG:cG:B, GG:c*:B, GG:*G:B, GG:**:B
  // 6. GG:cG:[NG]
  // 7. GG:c[GN]:B
  // 8. GG:c[BN]:[NG]
  const std::regex re1(R"([BNG][BNG]:[cd\*][BNG\*])");
  const std::regex re2(R"([BNG][BNG][cd]:[BNG])");
  const std::regex re3(R"([BNG][BNG]:[cd\*]\[([BNG]+)\])");
  const std::regex re4(R"([BNG][BNG][cd]:\[([BNG]+)\])");
  const std::regex re5(R"([BNG][BNG]:[cd\*][BNG\*]:[BNG])");
  const std::regex re6(R"([BNG][BNG]:[cd\*][BNG\*]:\[([BNG]+)\])");
  const std::regex re7(R"([BNG][BNG]:[cd\*]\[([BNG]+)\]:([BNG]))");
  const std::regex re8(R"([BNG][BNG]:[cd\*]\[([BNG]+)\]:\[([BNG]+)\])");
  for (const std::string& s: patterns) {
    std::smatch m;
    if (std::regex_match(s, re1)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (s[4] != '*' && std::get<1>(p) != c2r(s[4])) { return s; }
    }
    else if (std::regex_match(s, re2)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]), Z = c2r(s[4]);
      Action a = c2a(s[2]);
      if (g.rep_dynamics.RepAt(X, Y, a) != Z) { return s; }
    }
    else if (std::regex_match(s, m, re3)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (!IsIn( std::get<1>(p), m[1].str()) ) { return s; }
    }
    else if (std::regex_match(s, m, re4)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      Action a = c2a(s[2]);
      Reputation Z = g.rep_dynamics.RepAt(X, Y, a);
      if ( !IsIn(Z, m[1].str()) ) { return s; }
    }
    else if (std::regex_match(s, re5)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (s[4] != '*' && std::get<1>(p) != c2r(s[4])) { return s; }
      Action a_not = (std::get<0>(p) == Action::C) ? Action::D : Action::C;
      if (g.rep_dynamics.RepAt(X,Y,a_not) != c2r(s[6])) { return s; }
    }
    else if (std::regex_match(s, m, re6)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (s[4] != '*' && std::get<1>(p) != c2r(s[4])) { return s; }
      if ( !IsIn(std::get<2>(p), m[1].str()) ) { return s; }
    }
    else if (std::regex_match(s, m, re7)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (!IsIn(std::get<1>(p), m[1].str())) { return s; }
      if (std::get<2>(p) != c2r(m[2].str()[0]) ) { return s; }
    }
    else if (std::regex_match(s, m, re8)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (!IsIn(std::get<1>(p), m[1].str())) { return s; }
      if (!IsIn(std::get<2>(p), m[2].str())) { return s; }
    }
    else {
      std::cerr << s << std::endl;
      throw std::runtime_error("invalid pattern");
    }

  }
  return std::string();
}


std::string ClassifyType(Game& g) {
  const ReputationDynamics rd = g.rep_dynamics;
  const ActionRule ar = g.resident_ar;

  const Reputation B = Reputation::B, N = Reputation::N, G = Reputation::G;
  const Action D = Action::D, C = Action::C;

  // GGd => B
  if (rd.RepAt(G, G, D) != B) {
    throw std::runtime_error("must not happen");
  }

  // G population is dominant
  //   when GG => *G
  //   at most one of GN, NG, NN go to N
  bool G_dominant;
  {
    Reputation gg = std::get<1>(g.At(G, G));
    int num_n = 0;
    Reputation ng = std::get<1>(g.At(N, G));
    Reputation gn = std::get<1>(g.At(G, N));
    Reputation nn = std::get<1>(g.At(N, N));
    if (ng == N) num_n++;
    if (gn == N) num_n++;
    if (nn == N) num_n++;
    G_dominant = (gg == G && num_n < 2);
  };

  std::set<std::string> types;

  // type-1: leading-eight like
  if (
    Match(g, {"GG:cG:B", "GB:dG", "BG:cG:[BN]"}).empty()
    && G_dominant
    ) {
    types.insert("01. leading_eight");
  }
  // type-2: Bad players recover cooperation via being N
  if (
    Match(g, {"GG:cG:B", "GB:dG", "BG:cN", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert("02.1 B recovers via BG:cN->NG:cG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dG", "BG:cN", "NG:cN", "NN:cG"}).empty()
    && G_dominant
    ) {
    types.insert("02.2 B recovers via BG:cN->NN:cG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dG", "BG:cN", "NG:cN", "NN:dG"}).empty()
    && G_dominant
    ) {
    types.insert("02.3 B recovers via BG:cN->NN:dG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dG", "BG:cN", "NG:dG"}).empty()
    && G_dominant
    ) {
    types.insert("02.4 B recovers via BG:cN->NG:dG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dG", "BG:dN", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert("02.5 B recovers via BG:dN->NG:cG");
  }
  // type-3: punisher has N reputation. Bad players recover G by cooperating with G player.
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cG", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert("03.1 punisher becomes N. GB:dN->NG:cG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cG", "NG:dG"}).empty()
    && G_dominant
    ) {
    types.insert("03.2 punisher becomes N. GB:dN->NG:dG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cG", "NN:cG", "NG:*N"}).empty()
    && G_dominant
    ) {
    types.insert("03.3 punisher becomes N. GB:dN->NN:cG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cG", "NN:dG", "NG:*N"}).empty()
    && G_dominant
    ) {
    types.insert("03.4 punisher becomes N. GB:dN->NN:dG");
  }
  // type-3-5: punisher has B reputation.
  if (
    Match(g, {"GG:cG:B", "GB:dB", "BG:cG"}).empty()
    && G_dominant
    ) {
    types.insert("03.5 punisher becomse B. GB:dB->BG:cG");
  }

  // type-4: punisher has N reputation. Bad players recover G via being N.
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cN", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert("04.1.1 punisher becomes N. B recovers via being N. GB:dN->NG:cG, BG:cN->NG:cG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cN", "NG:dG"}).empty()
    && G_dominant
    ) {
    types.insert("04.1.2 punisher becomes N. B recovers via being N. GB:dN->NG:dG, BG:cN->NG:dG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cN", "NG:cN", "NN:dG"}).empty()
    && G_dominant
    ) {
    types.insert("04.2.1 punisher becomes N. B recovers via being N. GB:dN->NN:dG, BG:cN->NN:dG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dN", "BG:cN", "NG:cN", "NN:cG"}).empty()
    && G_dominant
    ) {
    types.insert("04.2.2 punisher becomes N. B recovers via being N. GB:dN->NN:cG, BG:cN->NN:cG");
  }
  // type-3-6: punisher has B reputation. Bad players recover G via being N.
  if (
    Match(g, {"GG:cG:B", "GB:dB", "BG:cN", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert("04.3.1 punisher becomes B. B recovers via being N. GB:dB->BG:cN->NG:cG");
  }
  if (
    Match(g, {"GG:cG:B", "GB:dB", "BG:cN", "NG:dG"}).empty()
    && G_dominant
    ) {
    types.insert("04.3.2 punisher becomes B. B recovers via being N. GB:dB->BG:cN->NG:dG");
  }

  // type-5: G and N works as G for the leading eight
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
      }).empty()
    && !G_dominant
    ) {
    types.insert("05. GN works as G for the leading eight.");
  }
  // type-6: G and N works as G for the leading eight, but punishment by N is not justified
  if (
    (
      Match(g, {
        "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:dB",
        "BG:c[GN]:B", "BN:c[GN]:B",
      }).empty()
    )
    && !G_dominant
    ) {
    types.insert("06. GN works as G. Punishment by N is not justified. NB:dB");
  }
  // type-7: G and N works as G for the leading eight, but B players cooperate only with either G or N
  //      punishment by N against B may not always be justified
  if (
      Match(g, {
        "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:d[GN]",
        "BG:c[GN]:B", "BN:dB",
      }).empty()
      && !G_dominant
    ) {
    types.insert("07.1.1 GN works as G. B defects against N. BN:dB");
  }
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:d[GN]", "NB:d[GN]",
      "BN:c[GN]:B", "BG:dB",
    }).empty()
    && !G_dominant
    ) {
    types.insert("07.1.2 GN works as G. B defects against G. BG:dB");
  }
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:d[GN]", "NB:dB",
      "BG:c[GN]:B", "BN:dB",
    }).empty()
    && !G_dominant
    ) {
    types.insert("07.2.1 GN works as G. B defects against N. Punishment by N is not justified. BN:dB, NB:dB");
  }
  if (
    Match(g, {
        "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:dB",
        "BN:c[GN]:B", "BG:dB",
      }).empty()
      && !G_dominant
    ) {
    types.insert("07.2.2 GN works as G. B defects against G. Punishment by N is not justified. BG:dB, NB:dB");
  }
  // type-8: G and N works as G for the leading eight, but N-N defects each other
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:d[GN]",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("08. GN works as G but NN defects. NN:d[GN]");
  }
  // type-9: G and N works as G for the leading eight, but N-N defects each other
  //         B cooperates with G but defects against N (similar to type 8 but differ in `BN`)
  //         defection of B against N does not always cause Bad reputation
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:d[GN]",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]:B", "BN:d*",
    }).empty()
    && !G_dominant
    ) {
    types.insert("09. GN works as G but NN defects. B defects against N. NN:d[GN], BN:d*");
  }
  // type-10: G and N works as G for the leading eight, but N-N defects each other
  //          punishment of N against B causes a Bad reputation
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:d[GN]",
      "GB:d[GN]", "NB:dB",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("10. GN works as G but NN defects. Punishment by N is not justified. NN:d[GN], NB:dB");
  }
  // type-11: G and N works as G for the leading eight, but N-N defects each other
  //          punishment of N against B causes a Bad reputation, and B defects against N
  //          hybrid of type 9 and 10
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:d[GN]",
      "GB:d[GN]", "NB:dB",
      "BG:c[GN]:B", "BN:d*",
    }).empty()
    && !G_dominant
    ) {
    types.insert("11. GN works as G but NN defects. Punishment by N is not justified. B defects against N. NN:d[GN], NB:dB, BN:d*");
  }
  // type-12: G and N works as G for the leading eight
  //          N can maintain G reputation when making a mistake.
  //          Only the punishment by N against B is justified.
  //          The highest reputation in this case is N since it has a right to punish others.
  //          a variant of type-6
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:dB", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.1 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:B, GB:dB");
  }
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:G", "NN:c[GN]:B",
      "GB:dB", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.2 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:G NN:c[GN]:B, GB:dB");
  }
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:G",
      "GB:dB", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.3 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:G, GB:dB");
  }
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:G", "NN:c[GN]:G",
      "GB:dB", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.4 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:G NN:c[GN]:G, GB:dB");
  }
  // type-13: G and N works as G for the leading eight
  //          B and recovers a good reputation by defecting against N.
  //          Even though AllD player can eventually gain G reputation, she must spent a long time in B reputation since N players are not frequent. Thus, being a defector does not pay off.
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:cN:B",
      "GB:dG", "NB:dG",
      "BG:cG:B", "BN:dG",
      "BB:dB:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert("13. GN works as G. B recovers by defecting against N. BN:d[GN]");
    // return 13;
  }


  std::string type;
  if (types.size() > 0) {
    if (types.size() > 1) {
      for (const std::string& t: types) {
        std::cerr << t << ' ';
      }
      std::cerr << std::endl;
      std::cerr << g.Inspect();
      std::cerr << "G_dominant: " << G_dominant << std::endl;
      throw std::runtime_error("duplicate types");
    }
    type = *types.begin();
  }
  else {
    std::cerr << std::endl;
    std::cerr << g.Inspect();
    std::cerr << "G_dominant: " << G_dominant << std::endl;
    throw std::runtime_error("unknown types");
  }

  return type;
}

void PrintHistogramRepDynamics(const std::vector<uint64_t> game_ids) {
  std::cout << "num_ESSs: " << game_ids.size() << std::endl;

  std::vector<uint64_t> rep_ids;
  for (size_t i = 0; i < game_ids.size(); i++) {
    rep_ids.emplace_back(game_ids[i] >> 9ull);
  }
  /*
  std::sort(rep_ids.begin(), rep_ids.end());
  rep_ids.erase( std::unique(rep_ids.begin(), rep_ids.end()), rep_ids.end() );
  std::cout << "# of unique Reputation Dynamics: " << rep_ids.size() << std::endl;
   */

  std::vector< std::array<size_t,3> > histo(18, {0,0,0});
  for (uint64_t id: rep_ids) {
    ReputationDynamics rd(id);
    if ( rd.RepAt(Reputation::G, Reputation::G, Action::D) != Reputation::B ) {
      throw std::runtime_error("must not happen");
    }
    for (size_t n = 0; n < 18; n++) {
      histo[n][static_cast<int>(rd.reputations[n])]++;
    }
  }

  // print results
  std::cout << "         :        B       N       G" << std::endl;
  for (size_t n = 0; n < 18; n++) {
    Reputation rep_d = static_cast<Reputation>(n/6);
    Reputation rep_r = static_cast<Reputation>((n/2) % 3);
    Action act = static_cast<Action>(n % 2);
    std::cout << "(" << rep_d << "->" << rep_r << "," << act << ") : "
              << std::setw(8) << histo[n][0]
              << std::setw(8) << histo[n][1]
              << std::setw(8) << histo[n][2] << std::endl;
  }
}

void PrintHistogramActionRules(const std::vector<uint64_t> game_ids) {
  std::cout << "Histogram of ActionRules: " << std::endl;
  std::vector< std::array<size_t,2> > histo(9, {0,0});
  for (size_t i = 0; i < game_ids.size(); i++) {
    uint64_t ar_id = (game_ids[i] & 511ull);
    ActionRule ar(ar_id);
    for (size_t n = 0; n < 9; n++) {
      histo[n][static_cast<int>(ar.actions[n])]++;
    }
  }
  for (size_t n = 0; n < 9; n++) {
    Reputation rep_d = static_cast<Reputation>(n/3);
    Reputation rep_r = static_cast<Reputation>(n%3);
    std::cout << "(" << rep_d << "->" << rep_r << ") :"
              << std::setw(8) << histo[n][0]
              << std::setw(8) << histo[n][1] << std::endl;
  }
}

void PrintHistogramPrescriptions(const std::vector<uint64_t> game_ids) {
  std::vector< std::array<size_t,2> > histo_next_act(9, {0,0});
  std::vector< std::array<size_t,3> > histo_next_rep(9, {0, 0, 0});
  std::vector< std::array<size_t,3> > histo_other_rep(9, {0, 0, 0});
  for (uint64_t id: game_ids) {
    Game g(0.02, 0.02, id);
    for (size_t n = 0; n < 9; n++) {
      Reputation donor = static_cast<Reputation >(n/3);
      Reputation recip = static_cast<Reputation >(n%3);
      auto p = g.At(donor, recip);
      histo_next_act[n][static_cast<int>(std::get<0>(p))]++;
      histo_next_rep[n][static_cast<int>(std::get<1>(p))]++;
      histo_other_rep[n][static_cast<int>(std::get<2>(p))]++;
    }
  }

  // print results
  std::cout << "       :        d       c|       B       N       G|       B       N       G|" << std::endl;
  for (size_t n = 0; n < 9; n++) {
    Reputation donor = static_cast<Reputation>(n/3);
    Reputation recip = static_cast<Reputation>(n%3);
    std::cout << "(" << donor << "->" << recip << ") : "
      << std::setw(8) << histo_next_act[n][0]
      << std::setw(8) << histo_next_act[n][1]
      << "|"
      << std::setw(8) << histo_next_rep[n][0]
      << std::setw(8) << histo_next_rep[n][1]
      << std::setw(8) << histo_next_rep[n][2]
      << "|"
      << std::setw(8) << histo_other_rep[n][0]
      << std::setw(8) << histo_other_rep[n][1]
      << std::setw(8) << histo_other_rep[n][2] << std::endl;
  }
}

void PrintHHisto(const std::array<HistoNormalBin,3>& h_histo) {
  assert( h_histo.size() == 3 );
  std::cout << h_histo.size() << ' ' << h_histo[0].bin << std::endl;
  std::cout << "H_B histo:" << std::endl;
  auto histo0 = h_histo[0].Frequency();
  for (const auto &keyval : histo0) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
  std::cout << "H_N histo:" << std::endl;
  auto histo1 = h_histo[1].Frequency();
  for (const auto &keyval : histo1) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
  std::cout << "H_G histo:" << std::endl;
  auto histo2 = h_histo[2].Frequency();
  for (const auto &keyval : histo2) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
}

int main(int argc, char* argv[]) {

  if (argc != 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <new_ESS_ids_file>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::ifstream fin(argv[1]);
  if (!fin) {
    std::cerr << "Failed to open file " << argv[1] << std::endl;
    throw std::runtime_error("failed to open file");
  }

  std::vector< std::tuple<uint64_t,double,double,double,double> > inputs;

  while(fin) {
    uint64_t org_gid,gid;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      inputs.emplace_back(std::make_tuple(gid,c_prob,h0,h1,h2));
    }
  }

  struct Output {
    std::map<std::string, std::vector<uint64_t> > game_ids;
    using histo3_t = std::array<HistoNormalBin,3>;
    std::map<std::string, histo3_t > h_histos;
    histo3_t & GetOrInit(const std::string& type) {
      const double bin = 0.05;
      if (h_histos.find(type) == h_histos.end()) {
        h_histos.insert( std::make_pair(type, histo3_t({bin, bin, bin})));
      }
      return h_histos.at(type);
    }
    void Merge(const Output& other) {
      for (auto kv: other.game_ids) {
        game_ids[kv.first].insert(game_ids[kv.first].begin(), kv.second.begin(), kv.second.end());
      }
      for (auto kv: other.h_histos) {
        histo3_t& h3 = GetOrInit(kv.first);
        h3[0].Merge( kv.second[0] );
        h3[1].Merge( kv.second[1] );
        h3[2].Merge( kv.second[2] );
      }
    }
  };

  int num_threads;
  #pragma omp parallel
  { num_threads = omp_get_num_threads(); };

  std::cerr << "num_threads: " << num_threads << std::endl;

  std::vector<Output> outs(num_threads);

  #pragma omp parallel for
  for (size_t i = 0; i < inputs.size(); i++) {
    uint64_t gid = std::get<0>(inputs[i]);
    double c_prob = std::get<1>(inputs[i]), h0 = std::get<2>(inputs[i]), h1 = std::get<3>(inputs[i]), h2 = std::get<4>(inputs[i]);
    Game g(0.02, 0.02, gid, c_prob, {h0, h1, h2} );
    std::string type = ClassifyType(g);
    int th = omp_get_thread_num();
    outs[th].game_ids[type].push_back(gid);
    Output::histo3_t & histos = outs[th].GetOrInit(type);
    histos.at(0).Add(h0);
    histos.at(1).Add(h1);
    histos.at(2).Add(h2);
  }

  Output out;
  for (size_t i = 0; i < num_threads; i++) {
    out.Merge( outs[i] );
  }

  for (auto kv: out.game_ids) {
    std::cout << "type: " << kv.first << ", " << kv.second.size() << std::endl;
  }

  if (out.game_ids.find("") != out.game_ids.end()) {
    std::ofstream fout("non_L8.txt");
    for (auto gid: out.game_ids.at("")) {
      fout << gid << std::endl;
    }
    fout.close();
  }

  for (auto kv: out.game_ids) {
    std::cout << "=================== TYPE " << kv.first << "====================" << std::endl;
    PrintHistogramPrescriptions(kv.second);
    // PrintHistogramRepDynamics(kv.second);
    // PrintHistogramActionRules(kv.second);
    PrintHHisto(out.GetOrInit(kv.first));
  }

  return 0;
}