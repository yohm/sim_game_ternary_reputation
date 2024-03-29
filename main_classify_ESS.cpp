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

#include "Strategy.hpp"
#include "Game.hpp"
#include "HistoNormalBin.hpp"
#include "Entry.hpp"


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
      if (g.strategy.rd.RepAt(X, Y, a) != Z) { return s; }
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
      Reputation Z = g.strategy.rd.RepAt(X, Y, a);
      if ( !IsIn(Z, m[1].str()) ) { return s; }
    }
    else if (std::regex_match(s, re5)) {
      Reputation X = c2r(s[0]), Y = c2r(s[1]);
      auto p = g.At(X, Y);
      if (s[3] != '*' && std::get<0>(p) != c2a(s[3])) { return s; }
      if (s[4] != '*' && std::get<1>(p) != c2r(s[4])) { return s; }
      Action a_not = (std::get<0>(p) == Action::C) ? Action::D : Action::C;
      if (g.strategy.rd.RepAt(X,Y,a_not) != c2r(s[6])) { return s; }
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


std::string ClassifyType(uint64_t game_id) {
  const Reputation B = Reputation::B, N = Reputation::N, G = Reputation::G;
  const Action D = Action::D, C = Action::C;

  std::string desc = "", key = "";

  Game g(1.0e-3, 1.0e-3, game_id);
  Game::v3d_t H_3 = g.ResidentEqReputation();
  Game g_5(1.0e-5, 1.0e-5, game_id);
  Game::v3d_t H_5 = g_5.ResidentEqReputation();

  const double hN_exponent = (std::log10(H_3[1]) - std::log10(H_5[1])) / 2.0;
  const double hB_exponent = (std::log10(H_3[0]) - std::log10(H_5[0])) / 2.0;
  const double defect_level_exponent = (std::log10(1.0 - g.ResidentCoopProb()) - std::log10(1.0 - g_5.ResidentCoopProb())) / 2.0;
  const double tol = 0.05;

  if (g.ResidentCoopProb() < 0.99) {
    return "C0. partial cooperation";
  }
  if ( std::abs(defect_level_exponent - 1.0) >= tol ) {
    IC(defect_level_exponent, g.ID());
    return "C01. unexpected defect_level scaling";
  }


  auto classify_by_recovery_C1 = [&g,&desc,&key]() {
    // how B recover G in C1 norms
    if (
      Match(g, {"BG:cG", "NG:*G"}).empty()
      )
    {
      key += "R11.";
      desc += ", BG:cG NG:*G (R11: B->G,N->G)";
    }
    else if (
      Match(g, {"BG:cG", "NG:*B"}).empty()
      )
    {
      key += "R12.";
      desc += ", BG:cG NG:*B (R12: N->B->G)";
    }
    else if (
      Match(g, {"BG:cN", "NG:cG"}).empty()
      )
    {
      key += "R21.";
      desc += ", BG:cN NG:cG (R21: B->N->G,cc)";
    }
    else if (
      Match(g, {"BG:cN", "NG:dG"}).empty()
      )
    {
      key += "R22.";
      desc += ", BG:cN NG:dG (R22: B->N->G,cd)";
    }
    else if (
      Match(g, {"BG:dN", "NG:cG"}).empty()
      )
    {
      key += "R23.";
      desc += ", BG:dN NG:cG (R23: B->N->G,dc)";
    }
    else if (
      Match(g, {"BG:*N", "NG:cN", "NN:*G"}).empty()
      )
    {
      key += "R24.";
      desc += ", BG:*N NG:cN NN:*G (R24: B->N,NN->G,dc)";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_punishment_C1 = [&g,&desc,&key]() {
    if (
      Match(g, {"GB:dG"}).empty()
      )
    {
      key += "P1.";
      desc += ", GB:dG (P1: G punisher is justified)";
    }
    else if (
      Match(g, {"GB:dN", "GN:dG"}).empty()
      )
    {
      key += "P21.";
      desc += ", GB:dN GN:dG (P21: G punisher becomes N, N is punished by G)";
    }
    else if (
      Match(g, {"GB:dN", "GN:cG"}).empty()
      )
    {
      key += "P22.";
      desc += ", GB:dN GN:cG (P22: G punisher becomes N, N is cooperated by G)";
    }
    else if (
      Match(g, {"GB:dN", "GN:cN", "NG:cG", "NN:*G"}).empty()
      )
    {
      key += "P23.";
      desc += ", GB:dN GN:cN NG:cG NN:*G (P23: G punisher becomes N, GN:cN, NG:cG & NN:*G)";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_recovery_C2 = [&g,&desc,&key]() {
    // how B recover G in C1 norms
    if (
      Match(g, {"BG:cG"}).empty()
      )
    {
      key += "R1.";
      desc += ", BG:cG (R1: B->G)";
    }
    else if (
      Match(g, {"BG:cN"}).empty()
      )
    {
      key += "R2.";
      desc += ", BG:cN (R2: B->N)";
    }
    else {
      key += "99.";
    }
  };
  auto classify_by_punishment_C2 = [&g,&desc,&key]() {
    if (
      Match(g, {"GB:dG"}).empty()
      )
    {
      key += "P1.";
      desc += ", GB:dG (P1: G punisher is justified)";
    }
    else if (
      Match(g, {"GB:dN"}).empty()
      )
    {
      key += "P2.";
      desc += ", GB:dN (P21: G punisher becomes N)";
    }
    else {
      key += "99.";
    }
  };

  auto classify_by_recovery_path_C3 = [&g,&desc,&key]() {
    // recovery path
    if (
      Match(g, {"BG:c[GN]:B", "BN:c[GN]:B"}).empty()
      ) {
      key += "R1.";
      desc += ", B[GN]:c[GN]:B (R1: B cooperates G&N)";
    } else if (
      Match(g, {"BG:c[GN]:B", "BN:dB:B"}).empty()
      ) {
      key += "R21.";
      desc += ", BN:dB:B or BG:c[GN]:B (R21: B cooperates with G but not with N)";
    } else if (
      Match(g, {"BG:dB:B", "BN:c[GN]:B"}).empty()
      ) {
      key += "R22.";
      desc += ", BN:c[GN]:B or BG:dB:B (R22: B cooperates with N but not with G)";
    } else {
      key += "99.";
      desc += ", unknown recovery pattern";
    }
  };

  auto classify_by_punishment_C3 = [&g,&desc,&key]() {  // for type2, classify of punisher
    if (
      Match(g, {"GB:d[GN]", "NB:d[GN]"}).empty()
      )
    {
      key += "P1.";
      desc += ", GB:d[GN] NB:d[GN] (P1: both GN punishers are justified)";
    }
    else if (
      Match(g, {"GB:d[GN]", "NB:dB"}).empty()
      )
    {
      key += "P21.";
      desc += ", GB:d[NG] NB:dB (P21: N pusniher is not justified)";
    }
    else if (
      Match(g, {"GB:dB", "NB:d[GN]"}).empty()
      )
    {
      key += "P22.";
      desc += ", GB:dB, NB:d[GN] (P22: G punisher is not justified)";
    }
    else {
      key += "99.";
    }
  };

  // C1
  if ( std::abs(hN_exponent - 1.0) < tol && std::abs(hB_exponent - 1.0) < tol ) {
    key += "C1.";
    desc += "h_N=O(mu),h_G=O(mu) (C1: G dominant)";

    classify_by_punishment_C1();
    classify_by_recovery_C1();
  }
  else if (
    std::abs(hN_exponent - 0.5) < tol && std::abs(hB_exponent - 1.0) < tol
    ) {
    if (
      Match(g, {"GG:cG", "GN:cG", "NG:cN", "NN:*G"}).empty()
      )
    {
      key += "C21.";
      desc += "h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cG,NG:cN,NN:*G)";
    }
    else if (
      Match(g, {"GG:cG", "GN:cN", "NG:cG", "NN:*G"}).empty()
      )
    {
      key += "C22.";
      desc += "h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*G)";
    }
    else if (
      Match(g, {"GG:cG", "GN:cN", "NG:cG", "NN:*B"}).empty()
      )
    {
      key += "C23.";
      desc += "h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu), GG:cG,GN:cN,NG:cG,NN:*B)";
    }
    else {
      key += "C29.";
      desc += "h_N=O(mu^1/2),h_B=O(mu) (C2: N~sqrt(mu))";

    }

    classify_by_punishment_C2();
    classify_by_recovery_C2();
  }
  else if (
    std::abs(hN_exponent - 0.0) < tol && std::abs(hB_exponent - 1.0) < tol
    ) {
    key += "C3.";
    desc += "h_N=O(1),h_G=O(mu) (C3: GN dominant)";

    classify_by_punishment_C3();
    classify_by_recovery_path_C3();
  }
  else if (
    std::abs(hN_exponent - 0.33) < tol && std::abs(hB_exponent - 0.67) < tol
    ) {
    key += "C4.";
    desc += "h_N=O(mu^1/3),h_B=O(mu^2/3) (C4: N~mu^1/3, B~mu^2/3)";

    classify_by_punishment_C1();
    classify_by_recovery_C1();
  }
  else {
    IC(hN_exponent, hB_exponent);
    key += "C99.";
  }

  return key + " " + desc;
}


struct Output {
  std::map<std::string, std::vector<Entry>> map_type_inputs;
  using histo3_t = std::array<HistoNormalBin,3>;
  std::map<std::string,histo3_t> MakeHistoH() const {
    std::map<std::string, histo3_t> h_histos;
    for (auto kv: map_type_inputs) {
      std::string type = kv.first;
      const double bin = 0.01;
      h_histos.insert( std::make_pair(type, histo3_t({bin, bin, bin})));
      for (const Entry& in : kv.second) {
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
  void SortByID() {
    for (auto kv: map_type_inputs) {
      std::sort(kv.second.begin(), kv.second.end());
    }
  }
};

void PrintHistogramPrescriptions(const std::vector<Entry>& inputs) {
  std::vector< std::array<size_t,2> > histo_next_act(9, {0,0});
  std::vector< std::array<size_t,3> > histo_next_rep(9, {0, 0, 0});
  std::vector< std::array<size_t,3> > histo_other_rep(9, {0, 0, 0});
  for (const Entry& input: inputs) {
    Game g(0.001, 0.001, input.gid);
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

void PrintContinuationPayoffOrders(const std::vector<Entry>& inputs) {
  std::map<std::string,int> order_count = {{"BNG", 0}, {"BGN", 0}, {"NBG", 0}, {"NGB", 0}, {"GBN", 0}, {"GNB", 0}};
  for (const Entry& input: inputs) {
    Game g(0.02, 0.02, input.gid, input.c_prob, input.h);
    auto cont = g.ContinuationPayoff(0.5, 2.0, 1.0, 0.02);
    if (cont[0] <= cont[1] && cont[1] <= cont[2]) { order_count.at("BNG") += 1; }
    else if(cont[0] <= cont[2] && cont[2] <= cont[1]) { order_count.at("BGN") += 1; }
    else if(cont[1] <= cont[0] && cont[0] <= cont[2]) { order_count.at("NBG") += 1; }
    else if(cont[1] <= cont[2] && cont[2] <= cont[0]) { order_count.at("NGB") += 1; }
    else if(cont[2] <= cont[0] && cont[0] <= cont[1]) { order_count.at("GBN") += 1; }
    else if(cont[2] <= cont[1] && cont[1] <= cont[0]) { order_count.at("GNB") += 1; }
    else { IC(cont); throw std::runtime_error("must not happen"); }
  }
  std::cout << "reputation order:\n";
  for (const auto& kv: order_count) {
    if (kv.second > 0) {
      std::cout << "  " << kv.first << ": " << kv.second << "\n";
    }
  }
}

void PrintHHisto(const std::array<HistoNormalBin,3>& h_histo) {
  assert( h_histo.size() == 3 );
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
  std::regex re(R"(^[CPR\d\.\-]+)");
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

  std::vector<Entry> inputs = Entry::LoadAndUniqSort(argv[1]);

  int num_threads;
  #pragma omp parallel shared(num_threads) default(none)
  { num_threads = omp_get_num_threads(); };

  std::cerr << "num_threads: " << num_threads << std::endl;

  std::vector<Output> outs(num_threads);

  #pragma omp parallel for shared(inputs,outs,std::cerr) default(none) schedule(dynamic, 1000)
  for (size_t i = 0; i < inputs.size(); i++) {
    if (inputs.size() > 20 && i % (inputs.size()/20) == 0) { std::cerr << "progress: " << (i*100)/inputs.size() << " %" << std::endl; }
    Entry input = inputs[i];
    std::string type = ClassifyType(input.gid);
    if (input.c_prob == -1.0) {
      // we need to calculate probs again
      Game g(1.0e-3, 1.0e-3, input.gid);
      input.c_prob = g.ResidentCoopProb();
      input.h = g.ResidentEqReputation();
    }
    int th = omp_get_thread_num();
    outs[th].map_type_inputs[type].emplace_back(input);
  }

  Output out;
  for (size_t i = 0; i < num_threads; i++) {
    out.Merge( outs[i] );
  }
  out.SortByID();

  for (auto kv: out.map_type_inputs) {
    std::cout << "type: " << kv.first << ", " << kv.second.size() << std::endl;
  }

  auto histo_map = out.MakeHistoH();
  for (auto kv: out.map_type_inputs) {
    std::cout << "=================== TYPE " << kv.first << "====================" << std::endl;
    PrintHistogramPrescriptions(kv.second);
    PrintContinuationPayoffOrders(kv.second);
    PrintHHisto(histo_map.at(kv.first));

    const int OUT_SIZE_TH = -1;
    std::string key = ExtractKeyFromType(kv.first);

    std::ofstream fout(std::string("ESS_") + key);
    int count = 0;
    for (const Entry& input: kv.second) {
      fout << input.gid << ' ' << input.c_prob << ' ' << input.h.at(0) << ' ' << input.h.at(1) << ' ' << input.h.at(2) << std::endl;
      count++;
      if (count == OUT_SIZE_TH) break;
    }
    fout.close();
  }

  return 0;
}