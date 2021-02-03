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
#include <icecream.hpp>

#include "ReputationDynamics.hpp"
#include "Game.hpp"
#include "HistoNormalBin.hpp"


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
    Match(g, {
      "GG:cG:B", "GB:dG", "BG:cN:B", "NG:cG:[BN]",
      }).empty()
    && G_dominant
    ) {
    types.insert("02.1 B recovers via BG:cN->NG:cG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dG", "BG:cN:B", "NG:cN:B", "NN:cG:B",
      "GN:cG:B", "NB:d*"
    }).empty()
    && G_dominant
    ) {
    types.insert("02.2 B recovers via BG:cN->NN:cG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dG", "BG:cN:B", "NG:cN:B", "NN:dG",
      "GN:cG:B", "NB:d*"
      }).empty()
    && G_dominant
    ) {
    types.insert("02.3 B recovers via BG:cN->NN:dG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dG", "BG:cN", "NG:dG",
      "GN:dG"
    }).empty()
    && G_dominant
    ) {
    types.insert("02.4 B recovers via BG:cN->NG:dG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dG", "BG:dN:[BN]", "NG:cG:[BN]",
      "GN:dG"
    }).empty()
    && G_dominant
    ) {
    types.insert("02.5 B recovers via BG:dN->NG:cG");
  }
  // type-3: punisher has N reputation. Bad players recover G by cooperating with G player.
  if (
    Match(g, {
      "GG:cG:B", "GB:dN", "BG:cG:[BN]", "NG:cG:[BN]",
      "GN:*[NG]"
      }).empty()
    && G_dominant
    ) {
    types.insert("03.1 punisher becomes N. GB:dN->NG:cG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dN", "BG:cG:B", "NG:dG",
      "GN:*G"
      }).empty()
    && G_dominant
    ) {
    types.insert("03.2 punisher becomes N. GB:dN->NG:dG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dN", "BG:cG:B", "NN:cG:B", "NG:cN:B",
      "GN:cG:B", "NB:d*", "BN:**:B"
      }).empty()
    && G_dominant
    ) {
    types.insert("03.3 punisher becomes N. GB:dN->NN:cG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dN", "BG:cG:B", "NN:dG", "NG:cN:B",
      "GN:cG:B", "NB:d[NG]", "BN:c[NG]:B"
      }).empty()
    && G_dominant
    ) {
    types.insert("03.4 punisher becomes N. GB:dN->NN:dG");
  }
  // type-3-5: punisher has B reputation.
  if (
    Match(g, {
      "GG:cG:B", "GB:dB:B", "BG:cG:B",
      "GN:cN:B", "NG:cG:B", "NN:cG:B", "NB:d[NG]",
      "BN:c[NG]:B"
    }).empty()
    && G_dominant
    ) {
    types.insert("03.5 punisher becomes B. GB:dB->BG:cG");
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
    Match(g, {
      "GG:cG:B", "GB:dN", "BG:cN:B", "NG:cN:B", "NN:dG",
      "GN:cG:B", "NB:d*", "BN:**:B"
    }).empty()
    && G_dominant
    ) {
    types.insert("04.2.1 punisher becomes N. B recovers via being N. GB:dN->NN:dG, BG:cN->NN:dG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GB:dN", "BG:cN", "NG:cN:B", "NN:cG:B",
      "GN:cG:B", "NB:d*", "BN:**:B"
    }).empty()
    && G_dominant
    ) {
    types.insert("04.2.2 punisher becomes N. B recovers via being N. GB:dN->NN:cG, BG:cN->NN:cG");
  }
  // type-3-6: punisher has B reputation. Bad players recover G via being N.
  if (
    Match(g, {
      "GG:cG:B", "GB:dB:B", "BG:cN:B", "NG:cG:B",
      "GN:cN:B", "NG:cG:B", "NN:cG:B", "BN:c[NG]:B"
    }).empty()
    && G_dominant
    ) {
    types.insert("04.3.1 punisher becomes B. B recovers via being N. GB:dB->BG:cN->NG:cG");
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
        "GG:cG:B", "GN:cN:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:dB:B",
        "BG:cG:B", "BN:c[GN]:B",
      }).empty()
    )
    && !G_dominant
    ) {
    types.insert("06.1.1 GN works as G. Punishment by N is not justified. NB:dB, GG:cG, GB:cG");
  }
  if (
    (
      Match(g, {
        "GG:cG:B", "GN:cN:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:dG", "NB:dB:B",
        "BG:cN:B", "BN:cG:B",
      }).empty()
    )
    && !G_dominant
    ) {
    types.insert("06.1.2 GN works as G. Punishment by N is not justified. NB:dB, GG:cG, GB:cN");
  }
  if (
    (
      Match(g, {
        "GG:cN:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:dB:B",
        "BG:cG:B", "BN:c[GN]:B",
      }).empty()
    )
    && !G_dominant
    ) {
    types.insert("06.2.1 GN works as G. Punishment by N is not justified. NB:dB, GG:cN, BG:cG");
  }
  if (
    (
      Match(g, {
        "GG:cN:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:dB:B",
        "BG:cN:B", "BN:c[GN]:B",
      }).empty()
    )
    && !G_dominant
    ) {
    types.insert("06.2.2 GN works as G. Punishment by N is not justified. NB:dB, GG:cN, BG:cN");
  }
  // type-7: G and N works as G for the leading eight, but B players cooperate only with either G or N
  //      punishment by N against B may not always be justified
  if (
      Match(g, {
        "GG:cG:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
        "GB:d[GN]", "NB:d[GN]",
        "BG:c[GN]:B", "BN:dB:B",
      }).empty()
      && !G_dominant
    ) {
    types.insert("07.1.1.1 GN works as G. B defects against N. BN:dB");
  }
  if (
    Match(g, {
      "GG:cN:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]:B", "BN:dB:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("07.1.1.2 GN works as G. B defects against N. BN:dB");
  }
  if (
    Match(g, {
      "GG:c[GN]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:d[GN]", "NB:d[GN]",
      "BN:c[GN]:B", "BG:dB:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("07.1.2 GN works as G. B defects against G. BG:dB");
  }
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:cN:B",
      "GB:dG", "NB:dB:B",
      "BG:cG:B", "BN:dB:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("07.2.1 GN works as G. B defects against N. Punishment by N is not justified. BN:dB, NB:dB");
  }
  // type-8: G and N works as G for the leading eight, but N-N defects each other
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dN",
      "GB:dG", "NB:dG",
      "BG:cG:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("08.1. GN works as G but NN defects. NN:d[GN], NB:dG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dN",
      "GB:dG", "NB:dN",
      "BG:cG:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("08.2. GN works as G but NN defects. NN:d[GN], NB:dN");
  }
  // type-9: G and N works as G for the leading eight, but N-N defects each other
  //         B cooperates with G but defects against N (similar to type 8 but differ in `BN`)
  //         defection of B against N does not always cause Bad reputation
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dN",
      "GB:dG", "NB:dG",
      "BG:cG:B", "BN:dB:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("09.1. GN works as G but NN defects. B defects against N. NN:d[GN], BN:d*, NB:dG");
  }
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dN",
      "GB:dG", "NB:dN",
      "BG:cG:B", "BN:dB:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("09.2. GN works as G but NN defects. B defects against N. NN:d[GN], BN:d*, NB:dN");
  }
  // type-10: G and N works as G for the leading eight, but N-N defects each other
  //          punishment of N against B causes a Bad reputation
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dN",
      "GB:dG", "NB:dB:B",
      "BG:cG:B", "BN:c[GN]:B",
      // BB:(dB:B|dN:*|dG:*|c[NG]:B)
    }).empty()
    && !G_dominant
    ) {
    types.insert("10. GN works as G but NN defects. Punishment by N is not justified. NN:d[GN], NB:dB");
  }
  // type-11: G and N works as G for the leading eight, but N-N defects each other
  //          punishment of N against B causes a Bad reputation, and B defects against N
  //          hybrid of type 9 and 10
  // Note: BB is limited to 9 patterns: dB:B, d[NG]:*, c[NG]:B
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dN",
      "GB:dG", "NB:dB:B",
      "BG:cG:B", "BN:dB:B",
      // BB:(dB:B|d[NG]:*|c[NG]:B)
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
      "GG:cG:B", "GN:cG:B", "NG:cN:[GB]", "NN:cN:[GB]",
      "GB:dB:B", "NB:dN",
      "BG:cG:B", "BN:cG:B",
      // BB:(cG:B|dG:[BG])
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.1.1 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:B, GB:dB");
  }
  if (
    Match(g, {
      "GG:cG:B", "GN:cN:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:dB:B", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.1.2 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:B, GB:dB");
  }
  if (
    Match(g, {
      "GG:cN:B", "GN:cG:B", "NG:c[GN]:B", "NN:c[GN]:B",
      "GB:dB:B", "NB:d[GN]",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.1.3 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:B, GB:dB");
  }
  if (
    Match(g, {
      "GG:cN:B", "GN:cN:B", "NG:cG:B", "NN:cG:B",
      "GB:dB:B", "NB:dN",
      "BG:cG:B", "BN:cG:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.1.4.1 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:B, GB:dB");
  }
  if (
    Match(g, {
      "GG:cN:B", "GN:cN:B", "NG:cG:B", "NN:cG:B",
      "GB:dB:B", "NB:dG",
      "BG:c[GN]:B", "BN:c[GN]:B",
    }).empty()
    && !G_dominant
    ) {
    types.insert("12.1.4.2 GN works as G. N becomes G even when making a mistake. Punishment by G is not justified. NG:c[GN]:B NN:c[GN]:B, GB:dB");
  }
  // ---------------------
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

std::string ClassifyType2(Game& g) {
  const ReputationDynamics rd = g.rep_dynamics;
  const ActionRule ar = g.resident_ar;

  const Reputation B = Reputation::B, N = Reputation::N, G = Reputation::G;
  const Action D = Action::D, C = Action::C;

  // GGd => B
  if (rd.RepAt(G, G, D) != B) {
    throw std::runtime_error("must not happen");
  }

  std::string desc = "", key = "";

  auto classify_by_reputation_change_when_meeting_G = [&g,&desc,&key]() {
    // how B,N change reputation when meeting G
    if (
      Match(g, {"BG:cG", "NG:*G"}).empty()   // B->G<-N
      )
    {
      key += "1.";
      desc += ", BG:cG, NG:*G (B->G<-N)";
    }
    else if (
      Match(g, {"BG:cG", "NG:*B"}).empty()  // N->B->G
      )
    {
      key += "2.";
      desc += ", BG:cG NG:*B (N->B->G)";
    }
    else if (
      Match(g, {"BG:*N", "NG:*G"}).empty() // B->N->G
      )
    {
      key += "3.";
      desc += ", BG:*N NG:*G (B->N->G)";
    }
    else if (
      Match(g, {"BG:cG", "NG:*N"}).empty() // B->G N
      )
    {
      key += "4.";
      desc += ", BG:cG NG:*N (B->G N)";
    }
    else if (
      Match(g, {"BG:*N", "NG:*N"}).empty()  // B->N G
      )
    {
      key += "5.";
      desc += ", BG:*N NG:*N (B->N G)";
    }
    else if (
      Match(g, {"BG:*B", "NG:*G"}).empty()  // B N->G
      )
    {
      key += "6.";
      desc += ", BG:*B NG:*G (B N->G)";
    }
    else if (
      Match(g, {"BG:*B", "NG:*N"}).empty()  // B N G
      )
    {
      key += "7.";
      desc += ", BG:*B NG:*N (B N G)";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_pusniment_pattern = [&g,&desc,&key]() {
    if (
      Match(g, {"GB:dG"}).empty()
      )
    {
      key += "1.";
      desc += ", GB:dG";
    }
    else if (
      Match(g, {"GB:dN"}).empty()
      )
    {
      key += "2.";
      desc += ", GB:dN";
    }
    else if (
      Match(g, {"GB:dB"}).empty()
      )
    {
      key += "3.";
      desc += ", GB:dB";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_recovery_when_meeting_NG = [&g,&desc,&key]() {
    // BN:*G, BG:*G
    if (
      Match(g, {"BN:*G", "BG:*G"}).empty()
      )
    {
      key += "1.";
      desc += ", (BN->G,BG->G)";
    }
    else if (
      Match(g, {"BN:*N", "BG:*G"}).empty()
      )
    {
      key += "2.";
      desc += ", (BN->N,BG->G)";
    }
    else if (
      Match(g, {"BN:*G", "BG:*N"}).empty()
      )
    {
      key += "3.";
      desc += ", (BN->G,BG->N)";
    }
    else if (
      Match(g, {"BN:*N", "BG:*N"}).empty()
      )
    {
      key += "4.";
      desc += ", (BN->N,BG->N)";
    }
    else if (
      Match(g, {"BN:*B", "BG:*[NG]"}).empty()
      )
    {
      key += "5.";
      desc += ", (BN->B,BG->[NG])";
    }
    else if (
      Match(g, {"BN:*[NG]", "BG:*B"}).empty()
      )
    {
      key += "6.";
      desc += ", (BN->[NG],BG->B)";
    }
    else {
      key += "99.";
    }
  };

  // classify by GG:cG:B or GG:cN:B
  if (
    Match(g, {"GG:cG:B", "GN:*[BG]", "NG:*[BG]"}).empty()  // G is dominant
    )
  {
    key += "1.";
    desc += "GG:cG GN:*[BG] NG:*[BG] (G dominant)";

    classify_by_reputation_change_when_meeting_G();
    classify_by_pusniment_pattern();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:*[BG]", "NG:*N", "NN:*[BG]"}).empty()  // N significant
    ) {
    key += "2.";
    desc += "GG:cG GN:*[BG] NG:*N NN:*[BG] (N minor)";

    // classify_by_reputation_change_when_meeting_G();
    // classify_by_pusniment_pattern();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:*[BG]", "NG:*N", "NN:*N"}).empty()  // N significant
    ) {
    key += "3.";
    desc += "GG:cG GN:*[BG] NG:*N NN:*N (N major)";

    /*
    if (
      Match(g, {"NB:*[BG]"}).empty()
      ) {
      key += "1.";
      desc += "NB:*[BG]";
    }
    else {
      key += "2.";
      desc += "NB:*N";
    }
     */
    // classify_by_reputation_change_when_meeting_G();
    // classify_by_pusniment_pattern();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:*N", "NG:*G", "NN:*[BG]"}).empty()  // N significant
    )
  {
    key += "4.";
    desc += "GG:cG:B, GN:*N NG:*G NN:*[BG] (N minor)";

    classify_by_reputation_change_when_meeting_G();
    classify_by_pusniment_pattern();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:*N", "NG:*G", "NN:*N"}).empty()  // N significant
    )
  {
    key += "5.";
    desc += "GG:cG:B, GN:*N NG:*G NN:*N (N major)";

    if (g.ResidentEqReputation()[2] > 0.7) {
      key += "1.";
      desc += ", h_G>0.7";
    }
    else {
      key += "2.";
      desc += ", h_G<0.7";
    }
    // classify_by_reputation_change_when_meeting_G();
    // classify_by_pusniment_pattern();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:cN", "NG:cN"}).empty()  // N more significant
    )
  {
    key += "6.";
    desc += "GG:cG:B GN:cN NG:cN (N major)";

    // classify_by_recovery_when_meeting_NG();
    // classify_by_pusniment_pattern();
  }
  else if (
    Match(g, {"GG:cN:B"}).empty()
    )
  {
    key += "7.";
    desc += "GG:cN:B (N major)";

    // classify_by_reputation_change_when_meeting_G();
    // classify_by_pusniment_pattern();
  }
  else {
    key += "99.";
  }

  return key + " " + desc;
}

std::string ClassifyType3(Game& g) {
  const ReputationDynamics rd = g.rep_dynamics;
  const ActionRule ar = g.resident_ar;

  const Reputation B = Reputation::B, N = Reputation::N, G = Reputation::G;
  const Action D = Action::D, C = Action::C;

  // GGd => B
  if (rd.RepAt(G, G, D) != B) {
    throw std::runtime_error("must not happen");
  }

  std::string desc = "", key = "";

  auto classify_by_reputation_change_when_meeting_G = [&g,&desc,&key]() {
    // how B,N change reputation when meeting G
    if (
      Match(g, {"BG:cG", "NG:*G"}).empty()   // B->G<-N
      )
    {
      key += "1.";
      desc += ", BG:cG, NG:*G (B->G<-N)";
    }
    else if (
      Match(g, {"BG:cG", "NG:*B"}).empty()  // N->B->G
      )
    {
      key += "2.";
      desc += ", BG:cG NG:*B (N->B->G)";
    }
    else if (
      Match(g, {"BG:*N", "NG:*G"}).empty() // B->N->G
      )
    {
      key += "3.";
      desc += ", BG:*N NG:*G (B->N->G)";
    }
    else if (
      Match(g, {"BG:cG", "NG:*N"}).empty() // B->G N
      )
    {
      key += "4.";
      desc += ", BG:cG NG:*N (B->G N)";
    }
    else if (
      Match(g, {"BG:*N", "NG:*N"}).empty()  // B->N G
      )
    {
      key += "5.";
      desc += ", BG:*N NG:*N (B->N G)";
    }
    else if (
      Match(g, {"BG:*B", "NG:*G"}).empty()  // B N->G
      )
    {
      key += "6.";
      desc += ", BG:*B NG:*G (B N->G)";
    }
    else if (
      Match(g, {"BG:*B", "NG:*N"}).empty()  // B N G
      )
    {
      key += "7.";
      desc += ", BG:*B NG:*N (B N G)";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_punishment_G = [&g,&desc,&key]() {
    if (
      Match(g, {"GB:dG"}).empty()
      )
    {
      key += "1.";
      desc += ", GB:dG";
    }
    else if (
      Match(g, {"GB:dN"}).empty()
      )
    {
      key += "2.";
      desc += ", GB:dN";
    }
    else if (
      Match(g, {"GB:dB"}).empty()
      )
    {
      key += "3.";
      desc += ", GB:dB";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_recovery_path_GN = [&g,&desc,&key]() {
    // recovery path
    if (
      Match(g, {"BG:c[GN]:B", "BN:c[GN]:B"}).empty()
      ) {
      key += "1.";
      desc += ", B[GN]:c[GN]:B (B cooperates G&N)";
    } else if (
      Match(g, {"BG:c[GN]:B", "BN:dB:B"}).empty() || Match(g, {"BG:dB:B", "BN:c[GN]:B"}).empty()
      ) {
      key += "2.";
      desc += ", BN:c[GN] or BG:c[GN] (B cooperates either GorN, defectors remain B)";
    } else if (
      Match(g, {"BG:c[GN]:B", "BN:d[GN]"}).empty()
      ) {
      key += "3.";
      desc += ", BN:c[GN] BN:d[GN] (B cooperates G, B may gain G when defecting N)";
    } else {
      key += "99.";
      desc += ", unknown recovery pattern";
    }
  };

  auto classify_by_punishment_GN = [&g,&desc,&key]() {  // for type2, classify of punisher
    if (
      Match(g, {"GB:d[GN]", "NB:d[GN]"}).empty()
      )
    {
      key += "1.";
      desc += ", GB:d[GN] NB:d[GN] (both GN punishers are justified)";
    }
    else if (
      Match(g, {"GB:d[GN]", "NB:dB"}).empty()
      )
    {
      key += "2.";
      desc += ", GB:d[NG] NB:dB (N pusniher is not justified)";
    }
    else if (
      Match(g, {"GB:dB", "NB:d[GN]"}).empty()
      )
    {
      key += "3.";
      desc += ", GB:dB, NB:d[GN] (G punisher is not justified)";
    }
    else {
      key += "99.";
    }
  };

  const std::array<double,3> H = g.ResidentEqReputation();

  // classify by GG:cG:B or GG:cN:B
  if (
    Match(g, {"GG:cG:B"}).empty() && H[1] < 0.1  // G is dominant
    )
  {
    key += "1.";
    desc += "GG:cG h_N<0.1 (G dominant)";

    classify_by_reputation_change_when_meeting_G();
    classify_by_punishment_G();
  }
  else if (
    Match(g, {"GG:c[NG]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B"}).empty()  // [NG][NG] forms cooperation
    ) {
    key += "2.";
    desc += "[GN][GN]:c[GN] (GN forms cooperation, defectors are B)";

    classify_by_recovery_path_GN();
    classify_by_punishment_GN();
  }
  else if ( // [NG][NG] forms cooperation but N has a higher value
    Match(g, {"GG:c[NG]:B", "GN:c[GN]:B", "NG:cN:G", "NN:cN:[BG]"}).empty() ||
    Match(g, {"GG:c[NG]:B", "GN:c[GN]:B", "NG:cN:[BG]", "NN:cN:G"}).empty()
    ) {
    key += "3.";
    desc += "[GN][GN]:c[GN]:[BG] (GN forms cooperation, defector becomes G)";
  }
  else if (
    Match(g, {"GG:c[NG]:B", "GN:c[GN]", "NG:c[GN]", "NN:d*"}).empty()  // [NG][NG] but NN forms cooperation
    ) {
    key += "4.";
    desc += "GG:c[GN] GN:c[GN] NG:c[GN] NN:d[GN]";
  }
  else {
    key += "99.";
  }

  return key + " " + desc;
}
struct Input {
  Input(uint64_t _gid, double _cprob, double h0, double h1, double h2) : gid(_gid), c_prob(_cprob), h({h0,h1,h2}) {};
  uint64_t gid;
  double c_prob;
  std::array<double,3> h;
};

struct Output {
  std::map<std::string, std::vector<Input>> map_type_inputs;
  using histo3_t = std::array<HistoNormalBin,3>;
  std::map<std::string,histo3_t> MakeHistoH() const {
    std::map<std::string, histo3_t> h_histos;
    for (auto kv: map_type_inputs) {
      std::string type = kv.first;
      const double bin = 0.05;
      h_histos.insert( std::make_pair(type, histo3_t({bin, bin, bin})));
      for (const Input& in : kv.second) {
        h_histos.at(type).at(0).Add(in.h.at(0));
        h_histos.at(type).at(1).Add(in.h.at(1));
        h_histos.at(type).at(2).Add(in.h.at(2));
      }
    }
    return std::move(h_histos);
  }
  void Merge(const Output& other) {
    for (auto kv: other.map_type_inputs) {
      map_type_inputs[kv.first].insert(map_type_inputs[kv.first].begin(), kv.second.begin(), kv.second.end());
    }
  }
};

void PrintHistogramPrescriptions(const std::vector<Input> inputs) {
  std::vector< std::array<size_t,2> > histo_next_act(9, {0,0});
  std::vector< std::array<size_t,3> > histo_next_rep(9, {0, 0, 0});
  std::vector< std::array<size_t,3> > histo_other_rep(9, {0, 0, 0});
  for (const Input& input: inputs) {
    Game g(0.02, 0.02, input.gid);
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
  std::cout << "       :        d       c|       B       N       G|       B       N       G" << std::endl;
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

std::string ExtractKeyFromType(const std::string& type) {
  std::regex re(R"(^[\d\.]+)");
  std::smatch m;
  if (std::regex_search(type, m, re) ) {
    return m[0].str();
  }
  else {
    std::cerr << "invalid type" << std::endl;
    throw std::runtime_error("failed to extract key from type");
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

  std::vector<Input> inputs;

  while(fin) {
    uint64_t org_gid,gid;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      inputs.emplace_back(gid, c_prob, h0, h1, h2);
    }
  }


  int num_threads;
  #pragma omp parallel shared(num_threads) default(none)
  { num_threads = omp_get_num_threads(); };

  std::cerr << "num_threads: " << num_threads << std::endl;

  std::vector<Output> outs(num_threads);

  #pragma omp parallel for shared(inputs,outs) default(none)
  for (size_t i = 0; i < inputs.size(); i++) {
    const Input& input = inputs[i];
    Game g(0.02, 0.02, input.gid, input.c_prob, input.h);
    std::string type = ClassifyType3(g);
    int th = omp_get_thread_num();
    outs[th].map_type_inputs[type].emplace_back(input);
  }

  Output out;
  for (size_t i = 0; i < num_threads; i++) {
    out.Merge( outs[i] );
  }

  for (auto kv: out.map_type_inputs) {
    std::cout << "type: " << kv.first << ", " << kv.second.size() << std::endl;
  }

  auto histo_map = out.MakeHistoH();
  for (auto kv: out.map_type_inputs) {
    std::cout << "=================== TYPE " << kv.first << "====================" << std::endl;
    PrintHistogramPrescriptions(kv.second);
    // PrintHistogramRepDynamics(kv.second);
    // PrintHistogramActionRules(kv.second);
    PrintHHisto(histo_map.at(kv.first));

    const size_t OUT_SIZE_TH = 1000;
    std::string key = ExtractKeyFromType(kv.first);
    std::ofstream fout(std::string("DP_") + key);
    size_t count = 0;
    for (const Input& input: kv.second) {
      fout << input.gid << ' ' << input.gid << ' ' << input.c_prob << ' ' << input.h.at(0) << ' ' << input.h.at(1) << ' ' << input.h.at(2) << std::endl;
      count++;
      if (count == OUT_SIZE_TH) break;
    }
    fout.close();
  }

  return 0;
}