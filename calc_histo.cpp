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
  // examples:
  // GG:cG
  // GGd:B
  // GG:c*
  // GB:*B
  // GB:*[NG]
  // BGd:[BN]
  const std::regex re1(R"([BNG][BNG]:[cd][BNG])");
  const std::regex re2(R"([BNG][BNG][cd]:[BNG])");
  const std::regex re3(R"([BNG][BNG]:[cd]\*)");
  const std::regex re4(R"([BNG][BNG]:\*[BNG])");
  const std::regex re5(R"([BNG][BNG]:\*\[([BNG]+)\])");
  const std::regex re6(R"([BNG][BNG]:[cd]\[([BNG]+)\])");
  const std::regex re7(R"([BNG][BNG][cd]:\[([BNG]+)\])");
  for (const std::string& s: patterns) {
    std::smatch m;
    if (std::regex_match(s, re1)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]), Z = c2r(s[4]);
      Action a = c2a(s[3]);
      if (g.At(X, Y) != std::make_pair(a, Z) ) { return s; }
    }
    else if (std::regex_match(s, re2)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]), Z = c2r(s[4]);
      Action a = c2a(s[2]);
      if (g.rep_dynamics.RepAt(X, Y, a) != Z) { return s; }
    }
    else if (std::regex_match(s, re3)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      Action a = c2a(s[3]);
      if (g.resident_ar.ActAt(X, Y) != a) { return s; }
    }
    else if (std::regex_match(s, re4)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]), Z = c2r(s[4]);
      if (g.At(X, Y).second != Z) { return s; }
    }
    else if (std::regex_match(s, m, re5)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      std::ssub_match sub = m[1];
      assert(sub.matched);
      bool ok = false;
      for (char c: sub.str()) {
        Reputation Z = c2r(c);
        if (p.second == Z) { ok = true; break; }
      }
      if (!ok) { return s; }
    }
    else if (std::regex_match(s, m, re6)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (p.first != c2a(s[3])) { return s; }
      std::ssub_match sub = m[1];
      assert(sub.matched);
      bool ok = false;
      for (char c: sub.str()) {
        Reputation Z = c2r(c);
        if (g.At(X,Y).second == Z) { ok = true; break; }
      }
      if (!ok) { return s; }
    }
    else if (std::regex_match(s, m, re7)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      Action a = c2a(s[2]);
      Reputation Z = g.rep_dynamics.RepAt(X, Y, a);

      std::ssub_match sub = m[1];
      assert(sub.matched);
      bool ok = false;
      for (char c: sub.str()) {
        Reputation P = c2r(c);
        if (Z == P) { ok = true; break; }
      }
      if (!ok) { return s; }
    }
    else {
      std::cerr << s << std::endl;
      throw std::runtime_error("invalid pattern");
    }

  }
  return std::string();
}


int ClassifyType(Game& g) {
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
    Reputation gg = g.At(G, G).second;
    int num_n = 0;
    Reputation ng = g.At(N, G).second;
    Reputation gn = g.At(G, N).second;
    Reputation nn = g.At(N, N).second;
    if (ng == N) num_n++;
    if (gn == N) num_n++;
    if (nn == N) num_n++;
    G_dominant = (gg == G && num_n < 2);
  };

  std::set<int> types;

  // type-1: leading-eight like
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dG", "BG:cG", "BGd:[BN]"}).empty()
    && G_dominant
    ) {
    types.insert(1);
  }
  // type-2: Bad players recover cooperation via being N
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dG", "BG:cN", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert(2);
  }
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dG", "BG:cN", "NG:cN", "NN:*G"}).empty()
    && G_dominant
    ) {
    types.insert(2);
  }
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dG", "BG:cN", "NG:dG"}).empty()
    && G_dominant
    ) {
    types.insert(2);
  }
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dG", "BG:dN", "NG:cG"}).empty()
    && G_dominant
    ) {
    types.insert(2);
  }
  // type-3: punisher has N reputation. Bad players recover G by cooperating with G player.
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dN", "BG:cG", "NG:*G"}).empty()
    && G_dominant
    ) {
    types.insert(3);
  }
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dN", "BG:cG", "NN:*G"}).empty()
    && G_dominant
    ) {
    types.insert(3);
  }
  // type-3-1: punisher has B reputation. Bad players recover G via being N.
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dB", "BG:cN", "NG:*G"}).empty()
    && G_dominant
    ) {
    types.insert(3);
  }
  // type-3-1: punisher has B reputation.
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dB", "BG:cG"}).empty()
    && G_dominant
    ) {
    types.insert(3);
  }

  // type-4: punisher has N reputation. Bad players recover G via being N.
  //    - GBd => N  // punisher has N reputation
  //    - BGc => N
  //    - BG => c
  //    - NG P_{NG} => G
  // ---
  // N population is minor
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dN", "BG:cN", "NG:*G"}).empty()
    && G_dominant
    ) {
    types.insert(4);
  }
  if (
    Match(g, {"GG:cG", "GGd:B", "GB:dN", "BG:cN", "NG:cN", "NN:*G"}).empty()
    && G_dominant
    ) {
    types.insert(4);
  }
  // type-5: G and N works as G for the leading eight
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:c[GN]",
      "GGd:B", "GNd:B", "NGd:B", "NNd:B",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]", "BN:c[GN]",
      "BGd:B", "BNd:B"
      }).empty()
    && !G_dominant
    ) {
    types.insert(5);
  }
  // type-6: G and N works as G for the leading eight, but punishment by N is not justified
  if (
    (
      Match(g, {
        "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:c[GN]",
        "GGd:B", "GNd:B", "NGd:B", "NNd:B",
        "GB:d[GN]", "NBd:B",
        "BG:c[GN]", "BN:c[GN]",
        "BGd:B", "BNd:B"
      }).empty()
    )
    && !G_dominant
    ) {
    types.insert(6);
  }
  // type-7: G and N works as G for the leading eight, but B players cooperate only with either G or N
  //      punishment by N against B may not always be justified
  if (
      Match(g, {
        "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:c[GN]",
        "GGd:B", "GNd:B", "NGd:B", "NNd:B",
        "GB:d[GN]",
        "BG:c[GN]", "BN:dB",
        "BGd:B"
      }).empty()
      && !G_dominant
    ) {
    types.insert(7);
  }
  if (
      Match(g, {
        "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:c[GN]",
        "GGd:B", "GNd:B", "NGd:B", "NNd:B",
        "GB:d[GN]",
        "BN:c[GN]", "BG:dB",
        "BNd:B"
      }).empty()
      && !G_dominant
    ) {
    types.insert(7);
  }
  // type-8: G and N works as G for the leading eight, but N-N defects each other
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:d[GN]",
      "GGd:B", "GNd:B", "NGd:B",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]", "BN:c[GN]",
      "BGd:B", "BNd:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert(8);
  }
  // type-9: G and N works as G for the leading eight, but N-N defects each other
  //         B cooperates with G but defects against N (similar to type 8 but differ in `BN`)
  //         defection of B against N does not always cause Bad reputation
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:d[GN]",
      "GGd:B", "GNd:B", "NGd:B",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]", "BN:d*",
      "BGd:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert(9);
  }
  // type-10: G and N works as G for the leading eight, but N-N defects each other
  //          punishment of N against B causes a Bad reputation
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:d[GN]",
      "GGd:B", "GNd:B", "NGd:B",
      "GB:d[GN]", "NB:dB",
      "BG:c[GN]", "BN:c[GN]",
      "BGd:B", "BNd:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert(10);
  }
  // type-11: G and N works as G for the leading eight, but N-N defects each other
  //          punishment of N against B causes a Bad reputation, and B defects against N
  //          hybrid of type 9 and 10
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:d[GN]",
      "GGd:B", "GNd:B", "NGd:B",
      "GB:d[GN]", "NB:dB",
      "BG:c[GN]", "BN:d*",
      "BGd:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert(11);
  }
  // type-12: G and N works as G for the leading eight
  //          N can maintain G reputation when making a mistake.
  //          Only the punishment by N against B is justified.
  //          The highest reputation in this case is N since it has a right to punish others.
  //          a variant of type-6
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:c[GN]",
      "GGd:B", "GNd:B", "NGd:[GB]", "NNd:[GB]",
      "GB:dB", "NB:d[GN]",
      "BG:c[GN]", "BN:c[GN]",
      "BGd:B", "BNd:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert(12);
  }
  // type-13: G and N works as G for the leading eight
  //          B and recovers a good reputation by defecting against N.
  //          Even though AllD player can eventually gain G reputation, she must spent a long time in B reputation since N players are not frequent. Thus, being a defector does not pay off.
  if (
    Match(g, {
      "GG:c[GN]", "GN:c[GN]", "NG:c[GN]", "NN:c[GN]",
      "GGd:B", "GNd:B", "NGd:B", "NNd:B",
      "GB:d[GN]", "NB:d[GN]",
      "BG:c[GN]", "BN:d[GN]",
      "BGd:B"
    }).empty()
    && !G_dominant
    ) {
    types.insert(13);
    // return 13;
  }

  int t = 0;
  if (types.size() > 0) {
    if (types.size() > 1) {
      for (int t: types) {
        std::cerr << t << ' ';
      }
      std::cerr << std::endl;
      std::cerr << g.Inspect();
      std::cerr << "G_dominant: " << G_dominant << std::endl;
      throw std::runtime_error("duplicate types");
    }
    t = *types.begin();
  }
  else {
    std::cerr << std::endl;
    std::cerr << g.Inspect();
    std::cerr << "G_dominant: " << G_dominant << std::endl;
    throw std::runtime_error("unknown types");
  }

  return t;
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

  const size_t N_TYPES = 14;
  const double bin = 0.05;
  using histo3_t = std::array<HistoNormalBin,3>;
  std::map<int, std::vector<uint64_t >> game_ids;
  std::map<int, histo3_t> h_histo;
  auto get_or_insert_h_histo = [&h_histo,bin](int i)->histo3_t& {
    if (h_histo.find(i) == h_histo.end()) {
      h_histo.insert( std::make_pair(i, histo3_t({bin, bin, bin}) ));
    }
    return h_histo.at(i);
  };

  while(fin) {
    uint64_t org_gid,gid;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      Game g(0.02, 0.02, gid, c_prob, {h0,h1,h2} );
      int i = ClassifyType(g);
      game_ids[i].push_back(gid);
      get_or_insert_h_histo(i).at(0).Add(h0);
      get_or_insert_h_histo(i).at(1).Add(h1);
      get_or_insert_h_histo(i).at(2).Add(h2);
    }
  }

  for (auto kv: game_ids) {
    std::cerr << "type: " << kv.first << ", " << kv.second.size() << std::endl;
  }

  if (game_ids.find(0) != game_ids.end()) {
    std::ofstream fout("non_L8.txt");
    for (auto gid: game_ids.at(0)) {
      fout << gid << std::endl;
    }
    fout.close();
  }

  for (auto kv: game_ids) {
    std::cout << "=================== TYPE " << kv.first << "====================" << std::endl;
    PrintHistogramRepDynamics(kv.second);
    PrintHistogramActionRules(kv.second);
    PrintHHisto(h_histo.at(kv.first));
  }

  return 0;
}