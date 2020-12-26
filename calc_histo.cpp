#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>

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
  std::map<double,double> Frequency() {
    std::map<double,double> result;
    int key_min = histo.begin()->first;
    int key_max = histo.rbegin()->first;
    for( int i = key_min; i <= key_max; i++ ) {
      double val = binidx_to_val( i );
      size_t n = ( histo.find(i) == histo.end() ) ? 0 : histo[i];
      double freq = n / binidx_to_binsize( i );
      result[val] = freq;
    }
    return result;
  }
  private:
  const double bin;
  std::function<int(double)> val_to_binidx = [=](double v)->int {
    return static_cast<int>( std::floor(v/bin) );
  };
  std::function<double(int)> binidx_to_val = [=](int i)->double {
    return i * bin;
  };
  std::function<double(int)> binidx_to_binsize = [=](int i)->double {
    return bin;
  };
  std::map<int, size_t> histo;
};



int main(int argc, char* argv[]) {

  if (argc != 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::ifstream fin(argv[1]);
  if (!fin) {
    std::cerr << "Failed to open file " << argv[1] << std::endl;
    throw std::runtime_error("failed to open file");
  }

  std::vector<uint64_t> game_ids;
  HistoNormalBin h0_histo(0.05);
  HistoNormalBin h1_histo(0.05);
  HistoNormalBin h2_histo(0.05);

  while(fin) {
    uint64_t org_gid,gid;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) { game_ids.push_back(gid); }
    h0_histo.Add(h0);
    h1_histo.Add(h1);
    h2_histo.Add(h2);
  }

  std::cerr << "Loaded " << game_ids.size() << " games" << std::endl;

  {
    std::vector<uint64_t> rep_ids;
    for (size_t i = 0; i < game_ids.size(); i++) {
      rep_ids.emplace_back(game_ids[i] >> 9ull);
    }
    std::sort(rep_ids.begin(), rep_ids.end());
    rep_ids.erase( std::unique(rep_ids.begin(), rep_ids.end()), rep_ids.end() );

    std::cout << "num_ESSs: " << game_ids.size() << std::endl
              << "num_RepDynamics: " << rep_ids.size() << std::endl;

    std::vector< std::array<size_t,3> > histo(18, {0,0,0});
    for (uint64_t id: rep_ids) {
      ReputationDynamics rd(id);
      for (size_t n = 0; n < 18; n++) {
        histo[n][static_cast<int>(rd.reputations[n])]++;
      }
    }
    for (size_t n = 0; n < 18; n++) {
      std::cout << n << " : " << histo[n][0] << ' ' << histo[n][1] << ' ' << histo[n][2] << std::endl;
    }
  }

  std::cout << "h0_histo:" << std::endl;
  auto histo0 = h0_histo.Frequency();
  for( const auto& keyval : histo0 ) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
  std::cout << "h1_histo:" << std::endl;
  auto histo1 = h1_histo.Frequency();
  for( const auto& keyval : histo1 ) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }
  std::cout << "h2_histo:" << std::endl;
  auto histo2 = h2_histo.Frequency();
  for( const auto& keyval : histo2 ) {
    std::cout << keyval.first << ' ' << keyval.second << std::endl;
  }

  return 0;
}