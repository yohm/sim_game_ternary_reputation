#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <set>
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
  std::map<double,double> Frequency() const {
    std::map<double,double> result;
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


int ClassifyType(const Game& g) {
  const ReputationDynamics rd = g.rep_dynamics;
  const ActionRule ar = g.resident_ar;

  const Reputation B = Reputation::B, N = Reputation::N, G = Reputation::G;
  const Action D = Action::D, C = Action::C;

  // GGd => B
  if (rd.RepAt(G, G, D) != B) {
    throw std::runtime_error("must not happen");
  }

  auto GorN = [](Reputation x)->bool { return (x == G || x == N); };

  std::set<int> types;

  // type-13: G and N works as G for the leading eight, and B defects N and recovers a good reputation.
  //          Even though AllD player can eventually gain G reputation, she must spent a long time in B reputation since N players are not frequent. Thus, being a defector does not pay off.
  // GGc => [GN], GNc => [GN], NGc => [GN], NNc => [GN]
  // GG => c, GN => c, NG => c, NN => c
  // GGd => B, GNd => B, NGd => B, NNd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => [GN]
  // BGc => [GN], BNd => [GN]
  // BG => c, BN => d
  // BGd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, C))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == C
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    && rd.RepAt(N, N, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    && GorN(rd.RepAt(N, B, D))
    // ---
    && GorN(rd.RepAt(B, G, C))
    && GorN(rd.RepAt(B, N, D))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == D
    // ---
    && rd.RepAt(B, G, D) == B
    ) {
    types.insert(13);
    // return 13;
  }
  // type-1: leading-eight like
  // GGc => G
  // GG => c
  // GGd => B
  // GB => d
  // ---
  // GBd => G
  // BGc => G
  // BG => c
  // BGd => !G
  if (
    rd.RepAt(G, G, C) == G
    && ar.ActAt(G, G) == C
    && rd.RepAt(G, G, D) == B
    && ar.ActAt(G, B) == D
    // ---------------
    && rd.RepAt(G, B, D) == G
    && rd.RepAt(B, G, C) == G
    && ar.ActAt(B, G) == C
    && rd.RepAt(B, G, D) != G
    ) {
    types.insert(1);
  }
  // type-2: Bad players recover cooperation via being N
  //    - GBd => G  // justification of the punishment
  //    - BG P_{BG} => N  // recovers cooperation via N
  //    - NG P_{NG} => G
  //    - BG => c or NG => c
  if (
    rd.RepAt(G, G, C) == G
    && ar.ActAt(G, G) == C
    && rd.RepAt(G, G, D) == B
    && ar.ActAt(G, B) == D
    // ---------------
    && rd.RepAt(G, B, D) == G
    && rd.RepAt(B, G, ar.ActAt(B, G)) == N
    && rd.RepAt(N, G, ar.ActAt(N, G)) == G
    && (ar.ActAt(B, G) == C || ar.ActAt(N, G) == C)
    ) {
    types.insert(2);
  }
  // type-3: punisher has N reputation. Bad players recover G by cooperating with G player.
  //    - GBd => N  // punisher has N reputation
  //    - BGc => G
  //    - BG => c
  //    - NG P_{NG} => G
  if (
    rd.RepAt(G, G, C) == G
    && ar.ActAt(G, G) == C
    && rd.RepAt(G, G, D) == B
    && ar.ActAt(G, B) == D
    // ---------------
    && rd.RepAt(G, B, D) == N
    && rd.RepAt(B, G, C) == G
    && ar.ActAt(B, G) == C
    && rd.RepAt(N, G, ar.ActAt(N, G)) == G
    ) {
    types.insert(3);
  }
  // type-4: punisher has N reputation. Bad players recover G via being N.
  //    - GBd => N  // punisher has N reputation
  //    - BGc => N
  //    - BG => c
  //    - NG P_{NG} => G
  if (
    rd.RepAt(G, G, C) == G
    && ar.ActAt(G, G) == C
    && rd.RepAt(G, G, D) == B
    && ar.ActAt(G, B) == D
    // ---------------
    && rd.RepAt(G, B, D) == N
    && rd.RepAt(B, G, C) == N
    && ar.ActAt(B, G) == C
    && rd.RepAt(N, G, ar.ActAt(N, G)) == G
    ) {
    types.insert(4);
  }
  // type-5: G and N works as G for the leading eight
  // GGc => [GN], GNc => [GN], NGc => [GN], NNc => [GN]
  // GG => c, GN => c, NG => c, NN => c
  // GGd => B, GNd => B, NGd => B, NNd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => [GN]
  // BGc => [GN], BNc => [GN]
  // BG => c, BN => c
  // BGd => B, BNd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, C))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == C
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    && rd.RepAt(N, N, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    && GorN(rd.RepAt(N, B, D))
    // ---
    && GorN(rd.RepAt(B, G, C))
    && GorN(rd.RepAt(B, N, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == C
    // ---
    && rd.RepAt(B, G, D) == B
    && rd.RepAt(B, N, D) == B
    ) {
    types.insert(5);
  }
  // type-6: G and N works as G for the leading eight, but punishment by either N or G player is not justified
  // GGc => [GN], GNc => [GN], NGc => [GN], NNc => [GN]
  // GG => c, GN => c, NG => c, NN => c
  // GGd => B, GNd => B, NGd => B, NNd => B
  // GB => d, NB => d
  // ---
  // (GBd => [GN] and NBd => B) || (NBd => [GN] and GBd => B)  // only difference from type 5
  // BGc => [GN], BNc => [GN]
  // BG => c, BN => c
  // BGd => B, BNd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, C))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == C
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    && rd.RepAt(N, N, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && (
      (
        GorN(rd.RepAt(G, B, D))
        && rd.RepAt(N, B, D) == B
      )
      ||
      (
        GorN(rd.RepAt(N, B, D))
        && rd.RepAt(G, B, D) == B
      )
    )
    // ---
    && GorN(rd.RepAt(B, G, C))
    && GorN(rd.RepAt(B, N, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == C
    // ---
    && rd.RepAt(B, G, D) == B
    && rd.RepAt(B, N, D) == B
    ) {
    types.insert(6);
  }
  // type-7: G and N works as G for the leading eight, but B players cooperate only with G or N
  //      punishment by N against B may not always be justified
  // GGc => [GN], GNc => [GN], NGc => [GN], NNc => [GN]
  // GG => c, GN => c, NG => c, NN => c
  // GGd => B, GNd => B, NGd => B, NNd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN]
  // (BG => c and BGc => [GN] and BN => d) or (BN => c and BNc => [GN] and BG => d)
  // BGd => B, BNd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, C))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == C
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    && rd.RepAt(N, N, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    // ---
    && (
      (
        ar.ActAt(B, G) == C
        && GorN(rd.RepAt(B, G, C))
        && ar.ActAt(B, N) == D
      )
      ||
      (
        ar.ActAt(B, N) == C
        && GorN(rd.RepAt(B, N, C))
        && ar.ActAt(B, G) == D
      )
    )
    // ---
    && rd.RepAt(B, G, D) == B
    && rd.RepAt(B, N, D) == B
    ) {
    types.insert(7);
  }
  // type-8: G and N works as G for the leading eight, but N-N defects each other
  // GGc => [GN], GNc => [GN], NGc => [GN], *NNd => [GN]*
  // GG => c, GN => c, NG => c, *NN => d*
  // GGd => B, GNd => B, NGd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => [GN]
  // BGc => [GN], BNc => [GN]
  // BG => c, BN => c
  // BGd => B, BNd => B
  if (
    rd.RepAt(G, G, C) == N
    // GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, D))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == D
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    && GorN(rd.RepAt(N, B, D))
    // ---
    && GorN(rd.RepAt(B, G, C))
    && GorN(rd.RepAt(B, N, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == C
    // ---
    && rd.RepAt(B, G, D) == B
    && rd.RepAt(B, N, D) == B
    ) {
    types.insert(8);
  }
  // type-9: G and N works as G for the leading eight, but N-N defects each other and B defects against N (similar to type 8 but differ in `BN`)
  //         defection of B against N does not always cause Bad reputation
  // GGc => N, GNc => [GN], NGc => [GN], NNd => [GN]
  // GG => c, GN => c, NG => c, NN => d
  // GGd => B, GNd => B, NGd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => [GN]
  // BGc => [GN]
  // BG => c, *BN => d*
  // BGd => B
  if (
    // GorN(rd.RepAt(G, G, C))
    rd.RepAt(G, G, C) == N
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, D))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == D
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    && GorN(rd.RepAt(N, B, D))
    // ---
    && GorN(rd.RepAt(B, G, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == D
    // ---
    && rd.RepAt(B, G, D) == B
    ) {
    types.insert(9);
  }
  // type-10: G and N works as G for the leading eight, but N-N defects each other and punishment of N against B causes a Bad reputation
  // GGc => [GN], GNc => [GN], NGc => [GN], NNd => [GN]
  // GG => c, GN => c, NG => c, NN => d
  // GGd => B, GNd => B, NGd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => B
  // BGc => [GN], BNc => [GN]
  // BG => c, BN => c
  // BGd => B, BNd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, D))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == D
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    && rd.RepAt(N, B, D) == B
    // ---
    && GorN(rd.RepAt(B, G, C))
    && GorN(rd.RepAt(B, N, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == C
    // ---
    && rd.RepAt(B, G, D) == B
    && rd.RepAt(B, N, D) == B
    ) {
    types.insert(10);
  }
  // type-11: G and N works as G for the leading eight, but N-N defects each other, and punishment of N against B causes a Bad reputation, and B defects against N
  //          hybrid of type 9 and 10
  // GGc => [GN], GNc => [GN], NGc => [GN], NNd => [GN]
  // GG => c, GN => c, NG => c, NN => d
  // GGd => B, GNd => B, NGd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => B
  // BGc => [GN]
  // BG => c, BN => c
  // BGd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, D))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == D
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && rd.RepAt(N, G, D) == B
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(G, B, D))
    && rd.RepAt(N, B, D) == B
    // ---
    && GorN(rd.RepAt(B, G, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == D
    // ---
    && rd.RepAt(B, G, D) == B
    ) {
    types.insert(11);
  }
  // type-12: G and N works as G for the leading eight, but N can maintain G reputation when making a mistake.
  //          Only the punishment by N against B is justified.
  //          The highest reputation in this case is N since it has a right to punish others.
  //          a variant of type-6
  // GGc => [GN], GNc => [GN], NGc => [GN], NNc => [GN]
  // GG => c, GN => c, NG => c, NN => c
  // GGd => B, GNd => B, **NGd => [GB], NNd => [GB]**
  // GB => d, NB => d
  // ---
  // NBd => [GN] and GBd => B  **
  // BGc => [GN], BNc => [GN]
  // BG => c, BN => c
  // BGd => B, BNd => B
  if (
    GorN(rd.RepAt(G, G, C))
    && GorN(rd.RepAt(G, N, C))
    && GorN(rd.RepAt(N, G, C))
    && GorN(rd.RepAt(N, N, C))
    // ---
    && ar.ActAt(G, G) == C
    && ar.ActAt(G, N) == C
    && ar.ActAt(N, G) == C
    && ar.ActAt(N, N) == C
    // ---
    && rd.RepAt(G, G, D) == B
    && rd.RepAt(G, N, D) == B
    && (rd.RepAt(N, G, D) == G || rd.RepAt(N, G, D) == B)
    && (rd.RepAt(N, N, D) == G || rd.RepAt(N, N, D) == B)
    // ---
    && ar.ActAt(G, B) == D
    && ar.ActAt(N, B) == D
    // ---------------
    && GorN(rd.RepAt(N, B, D))
    && rd.RepAt(G, B, D) == B
    // ---
    && GorN(rd.RepAt(B, G, C))
    && GorN(rd.RepAt(B, N, C))
    // ---
    && ar.ActAt(B, G) == C
    && ar.ActAt(B, N) == C
    // ---
    && rd.RepAt(B, G, D) == B
    && rd.RepAt(B, N, D) == B
    ) {
    types.insert(12);
  }

  int t = 0;
  if (types.size() > 1) {
    for (int t: types) {
      std::cerr << t << ' ';
    }
    std::cerr << std::endl;
    t = *types.begin();
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

void PrintHHisto(const std::vector<HistoNormalBin>& h_histo) {
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
  using h_histo_t = std::vector<HistoNormalBin>;
  std::vector<std::vector<uint64_t >> game_ids(N_TYPES);
  std::vector<h_histo_t> h_histo(N_TYPES, h_histo_t(3, bin));

  while(fin) {
    uint64_t org_gid,gid;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      Game g(0.02, 0.02, gid, c_prob, {h0,h1,h2} );
      int i = ClassifyType(g);
      game_ids.at(i).push_back(gid);
      h_histo.at(i).at(0).Add(h0);
      h_histo.at(i).at(1).Add(h1);
      h_histo.at(i).at(2).Add(h2);
    }
  }

  for (size_t i = 0; i < game_ids.size(); i++) {
    std::cerr << "type: " << i << ", " << game_ids.at(i).size() << std::endl;
  }

  if (game_ids[0].size() > 0) {
    std::ofstream fout("non_L8.txt");
    for (auto gid: game_ids.at(0)) {
      fout << gid << std::endl;
    }
    fout.close();
  }

  for (size_t i = 1; i < N_TYPES; i++) {
    std::cout << "=================== TYPE " << i << "====================" << std::endl;
    PrintHistogramRepDynamics(game_ids.at(i));
    PrintHistogramActionRules(game_ids.at(i));
    PrintHHisto(h_histo.at(i));
  }

  return 0;
}