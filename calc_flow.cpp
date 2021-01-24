#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <tuple>
#include <iomanip>

#include "Game.hpp"
#include "PopulationFlow.hpp"
#include "HistoNormalBin.hpp"

PopulationFlow GetRepFlow(uint64_t gid) {
  const double mu_e = 0.02, mu_a = 0.02;
  Game g(mu_e, mu_a, gid);
  g.ResidentEqReputation();
  return PopulationFlow(g);
}

void test_PopulationFlow() {
  {
    Game g(0.02, 0.02, 137863130404);  // leading-eight like strategy
    PopulationFlow rf(g);
    std::cout << g.Inspect() << rf.Inspect();
  }

  {
    Game g(0.02, 0.02, 140323977652);  // type 13
    PopulationFlow rf(g);
    std::cout << g.Inspect() << rf.Inspect();
  }

  {
    PopulationFlow rf1 = GetRepFlow(137863130404), rf2 = GetRepFlow(140323977652);
    std::cout << "D: " << rf1.D1(rf2) << ' ' << std::sqrt(rf1.D2(rf2)) << std::endl;
  }

  {
    std::vector<uint64_t> gids = {140322844084, 141139137460, 140325083572, 141140271028, 141956564404, 73671208375};
    PopulationFlow rf = GetRepFlow(140323977652);
    for (auto gid: gids) {
      PopulationFlow other = GetRepFlow(gid);
      std::cout << "D: " << rf.D1(other) << ' ' << std::sqrt(rf.D2(other)) << std::endl;
    }
  }
}

template <typename ... Args>
std::string format(const std::string& fmt, Args ... args )
{
  size_t len = std::snprintf( nullptr, 0, fmt.c_str(), args ... );
  std::vector<char> buf(len + 1);
  std::snprintf(&buf[0], len + 1, fmt.c_str(), args ... );
  return std::string(&buf[0], &buf[0] + len);
}

void CalcDs(uint64_t gid, const std::string& fname) {
  PopulationFlow rf = GetRepFlow(gid);

  std::ifstream fin(fname);
  std::vector<uint64_t> inputs;

  HistoNormalBin histo0(0.002); //, histo1(0.01), histo2(0.01);
  // std::string fname1 = format("TYPE_%lld", gid);
  // std::string fname2 = format("TYPE_N_%lld", gid);
  // std::ofstream fout1(fname1), fout2(fname2);
  constexpr double th_d = 0.03;

  while(fin) {
    uint64_t org_gid,gid2;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid2 >> c_prob >> h0 >> h1 >> h2;
    PopulationFlow other = GetRepFlow(gid2);
    auto d_pair = rf.MaxD(other);
    /*
    if (d_pair.first < th_d) {
      fout1 << org_gid << ' ' << gid2 << ' ' << c_prob << ' ' << h0 << ' ' << h1 << ' ' << h2 << ' ' << std::endl;
    }
    else {
      fout2 << org_gid << ' ' << gid2 << ' ' << c_prob << ' ' << h0 << ' ' << h1 << ' ' << h2 << ' ' << std::endl;
    }
     */
    histo0.Add(d_pair.first);
    /*
    if (d_pair.first > 0.03 && d_pair.first < 0.04) {
      Game g(0.02, 0.02, gid), g2(0.02, 0.02, gid2);
      std::cout << d_pair.first << ' ' << rf.RepString(d_pair.second) << std::endl;
      std::cout << g.Inspect() << g2.Inspect();
      break;
    }
     */
  }

  std::cout << "======== D0 histo:" << std::endl;
  for (const auto &keyval : histo0.Frequency()) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
  /*
  std::cout << "======== D1 histo:" << std::endl;
  for (const auto &keyval : histo1.Frequency()) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
  std::cout << "======== D2 histo:" << std::endl;
  for (const auto &keyval : histo2.Frequency()) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
   */
}

int main(int argc, char* argv[]) {
  // test_ReputationFlow();

  if (argc == 3) {
    CalcDs(std::stoull(argv[1]), argv[2]);
  }
  else if (argc == 2) {
    PopulationFlow rf = GetRepFlow(std::stoull(argv[1]));
    std::cerr << rf.Inspect();
  }
  else {
    std::cerr << "Usage: " << argv[0] << " <base_gid> [fname]" << std::endl;
    throw std::runtime_error("invalid usage");
  }

  return 0;
}