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


std::string ClassifyType(const Game& g) {
  const ReputationDynamics rd = g.rep_dynamics;
  const ActionRule ar = g.resident_ar;

  const Reputation B = Reputation::B, N = Reputation::N, G = Reputation::G;
  const Action D = Action::D, C = Action::C;

  // GGd => B
  if (rd.RepAt(G, G, D) != B) {
    throw std::runtime_error("must not happen");
  }

  std::string desc = "", key = "";

  auto classify_by_apology_to_G = [&g,&desc,&key]() {
    // how B,N change reputation when meeting G
    if (
      Match(g, {"BG:cG"}).empty()
      )
    {
      key += "A1.";
      desc += ", BG:cG (A1: B->G)";
    }
    else if (
      Match(g, {"BG:*N"}).empty()
      )
    {
      key += "A2.";
      desc += ", BG:*N (A2: B->N)";
    }
    else if (
      Match(g, {"BG:*B"}).empty()
      )
    {
      key += "A3.";
      desc += ", BG:*B (A3: B)";
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
      key += "P1.";
      desc += ", GB:dG (P1: G punisher is justified)";
    }
    else if (
      Match(g, {"GB:dN"}).empty()
      )
    {
      key += "P2.";
      desc += ", GB:dN (P2: G punisher becomes N)";
    }
    else if (
      Match(g, {"GB:dB"}).empty()
      )
    {
      key += "P3.";
      desc += ", GB:dB (P3: G punisher becomes B)";
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
      key += "A1.";
      desc += ", B[GN]:c[GN]:B (A1: B cooperates G&N)";
    } else if (
      Match(g, {"BG:c[GN]:B", "BN:dB:B"}).empty()
      ) {
      key += "A2.";
      desc += ", BN:dB:B or BG:c[GN]:B (A2: B cooperates with G but not with N)";
    } else if (
      Match(g, {"BG:dB:B", "BN:c[GN]:B"}).empty()
      ) {
      key += "A3.";
      desc += ", BN:c[GN]:B or BG:dB:B (A3: B cooperates with N but not with G)";
    } else if (
      Match(g, {"BG:c[GN]:B", "BN:d[GN]"}).empty()
      ) {
      key += "A4.";
      desc += ", BG:c[GN] BN:d[GN] (A4: B cooperates G not with N, B may gain G when defecting N)";
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
      key += "P1.";
      desc += ", GB:d[GN] NB:d[GN] (P1: both GN punishers are justified)";
    }
    else if (
      Match(g, {"GB:d[GN]", "NB:dB"}).empty()
      )
    {
      key += "P2.";
      desc += ", GB:d[NG] NB:dB (P2: N pusniher is not justified)";
    }
    else if (
      Match(g, {"GB:dB", "NB:d[GN]"}).empty()
      )
    {
      key += "P3.";
      desc += ", GB:dB, NB:d[GN] (P3: G punisher is not justified)";
    }
    else {
      key += "99.";
    }
  };

  const std::array<double,3> H = g.ResidentEqReputation();

  // classify by GG:cG:B or GG:cN:B
  if (
    /* Match(g, {"GG:cG:B"}).empty() && */ H[1] < 0.1  // G is dominant
    )
  {
    key += "C1.";
    desc += "GG:cG:B h_N<0.1 (C1: G dominant)";

    classify_by_punishment_G();
    classify_by_apology_to_G();
  }
  else if (
    Match(g, {"GG:c[NG]:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:c[GN]:B"}).empty()  // [NG][NG] forms cooperation
    ) {
    key += "C2.";
    desc += "[GN][GN]:c[GN]:B (C2: GN cooperation, defector gets B)";

    classify_by_punishment_GN();
    classify_by_recovery_path_GN();
  }
  else if ( // [NG][NG] forms cooperation but N has a higher value
    Match(g, {"GG:cG:B", "GN:cG:B", "NG:cN:G", "NN:cN:[GB]"}).empty() ||
    Match(g, {"GG:cG:B", "GN:cG:B", "NG:cN:[GB]", "NN:cN:G"}).empty()
    ) {
    key += "C3.";
    desc += "[GN][GN]:c[GN]:[BG] (C3: GN cooperation, defecting N gets G)";

    classify_by_punishment_GN();
    classify_by_recovery_path_GN();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:dG"}).empty()
    ) {
    key += "C4.";
    desc += "GG:cG:B [GN,NG]:c[GN]:B NN:dG (C4: NN defects and gets G)";

    classify_by_punishment_G();
    classify_by_apology_to_G();

  }
  else if (
    Match(g, {"GG:cG:B", "GN:c[GN]:B", "NG:c[GN]:B", "NN:dN"}).empty()
    ) {
    key += "C5.";
    desc += "GG:cG:B [GN,NG]:c[GN]:B NN:dN (C5: NN defects and gets N)";

    classify_by_punishment_G();
    classify_by_apology_to_G();
  }
  else if (
    Match(g, {"GG:cG:B", "GN:cN:B", "NG:cG:B", "NN:dB"}).empty()  // [NG][NG] but NN forms cooperation
    ) {
    key += "C6.";
    desc += "[GN][GN]:c[GN]:B NN:dB (C6: NN defects and gets B)";

    classify_by_punishment_G();
    classify_by_apology_to_G();
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

void PrintContinuationPayoffOrders(const std::vector<Input> inputs) {
  std::map<std::string,int> order_count = {{"BNG", 0}, {"BGN", 0}, {"NBG", 0}, {"NGB", 0}, {"GBN", 0}, {"GNB", 0}};
  for (const Input& input: inputs) {
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
  std::regex re(R"(^[CPA\d\.\-]+)");
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
    std::string type = ClassifyType(g);
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
    PrintContinuationPayoffOrders(kv.second);
    PrintHHisto(histo_map.at(kv.first));

    const int OUT_SIZE_TH = -1;
    std::string key = ExtractKeyFromType(kv.first);
    std::ofstream fout(std::string("DP_") + key);
    int count = 0;
    for (const Input& input: kv.second) {
      fout << input.gid << ' ' << input.gid << ' ' << input.c_prob << ' ' << input.h.at(0) << ' ' << input.h.at(1) << ' ' << input.h.at(2) << std::endl;
      count++;
      if (count == OUT_SIZE_TH) break;
    }
    fout.close();
  }

  return 0;
}