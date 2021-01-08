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


int ClassifyType(const Game& g) {
  ReputationDynamics rd = g.rep_dynamics;
  ActionRule ar = g.resident_ar;

  // -1: unknown
  //  1: leading-eight like
  //    - GGc => G
  //    - GG => c
  //    - GGd => B
  //    - GB => d
  //    - GBd => G
  //    ===
  //    - BGc => G
  //    - BG => c
  //    - BGd => B
  //    - NG P_{NG} => G
  // 2: two-step recovery (B->N->G)
  //    - GGc => G
  //    - GG => c
  //    - GGd => B
  //    - GB => d
  //    - GBd => G
  //    ===
  //    - BG P_{BG} => N  // recovers cooperation via N
  //    - NG P_{NG} => G
  //    - BG => c or NG => c
  // 3: punisher is denoted as N
  //    - GGc => G
  //    - GG => c
  //    - GGd => B
  //    - GB => d
  //    - GBd => N  // N indicates that the player is punishing
  //    ===
  //    - BGc => G
  //    - NGd => G
  //    - BG => c
  //    - NG => d


  // GGd => B
  if (rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::N) {
    rd = rd.Permute({1,0,2});  // swap N and B. Regard N as B
    ar = ar.Permute({1,0,2});
  }
  else if (rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::G) {
    throw std::runtime_error("GGd cannot be G");
  }


  /*
  // GGc => G and GG => c
  if (rd.RepAt(Reputation::G, Reputation::G, Action::C) != Reputation::G ) { return -1; }
  if (ar.ActAt(Reputation::G, Reputation::G) == Action::D) { return -1; }
  // GGd => B
  if (rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::G) {
    return -1;
  }
  else {
    if (rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::N) {
      rd = rd.Permute({1,0,2});  // swap N and B. Regard N as B
      ar = ar.Permute({1,0,2});
    }
  }
  // GB => d
  if (ar.ActAt(Reputation::G, Reputation::B) != Action::D) { return -1; }
   */

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
    rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    // ---------------
    && rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G
    && rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) != Reputation::G
    ) {
    return 1;
  }
  // type-2: Bad players recover cooperation via being N
  //    - GBd => G  // justification of the punishment
  //    - BG P_{BG} => N  // recovers cooperation via N
  //    - NG P_{NG} => G
  //    - BG => c or NG => c
  else if (
    rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    // ---------------
    && rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G
    && rd.RepAt(Reputation::B, Reputation::G, ar.ActAt(Reputation::B, Reputation::G)) == Reputation::N
    && rd.RepAt(Reputation::N, Reputation::G, ar.ActAt(Reputation::N, Reputation::G)) == Reputation::G
    && (ar.ActAt(Reputation::B, Reputation::G) == Action::C || ar.ActAt(Reputation::N, Reputation::G) == Action::C)
    ) {
    return 2;
  }
  // type-3: punisher has N reputation. Bad players recover G by cooperating with G player.
  //    - GBd => N  // punisher has N reputation
  //    - BGc => G
  //    - BG => c
  //    - NG P_{NG} => G
  else if (
    rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    // ---------------
    && rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N
    && rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && rd.RepAt(Reputation::N, Reputation::G, ar.ActAt(Reputation::N, Reputation::G)) == Reputation::G
    ) {
    return 3;
  }
  // type-4: punisher has N reputation. Bad players recover G via being N.
  //    - GBd => N  // punisher has N reputation
  //    - BGc => N
  //    - BG => c
  //    - NG P_{NG} => G
  else if (
    rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    // ---------------
    && rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N
    && rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && rd.RepAt(Reputation::N, Reputation::G, ar.ActAt(Reputation::N, Reputation::G)) == Reputation::G
    ) {
    return 4;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::N)
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::B
    ) {
    return 5;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (
      (
        (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
        && rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::B
      )
      ||
      (
        (rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::N)
        && rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::B
      )
    )
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::B
    ) {
    return 6;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    // ---
    && (
      (
        ar.ActAt(Reputation::B, Reputation::G) == Action::C
        && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
        && ar.ActAt(Reputation::B, Reputation::N) == Action::D
      )
      ||
      (
        ar.ActAt(Reputation::B, Reputation::N) == Action::C
        && (rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::N)
        && ar.ActAt(Reputation::B, Reputation::G) == Action::D
      )
    )
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::B
    ) {
    return 7;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::N)
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::B
    ) {
    return 8;
  }
  // type-9: G and N works as G for the leading eight, but N-N defects each other and B defects against N (similar to type 8 but differ in `BN`)
  //         defection of B against N does not always cause Bad reputation
  // GGc => [GN], GNc => [GN], NGc => [GN], NNd => [GN]
  // GG => c, GN => c, NG => c, NN => d
  // GGd => B, GNd => B, NGd => B
  // GB => d, NB => d
  // ---
  // GBd => [GN], NBd => [GN]
  // BGc => [GN]
  // BG => c, *BN => d*
  // BGd => B
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::N)
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    ) {
    return 9;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    && rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::B
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::B
    ) {
    return 10;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    && rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::B
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    ) {
    return 11;
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && (rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::B)
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::N)
    && rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::B
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::B
    ) {
    return 12;
  }
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
  else if (
    (rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::G, Reputation::N, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::G || rd.RepAt(Reputation::N, Reputation::N, Action::C) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::G, Reputation::G) == Action::C
    && ar.ActAt(Reputation::G, Reputation::N) == Action::C
    && ar.ActAt(Reputation::N, Reputation::G) == Action::C
    && ar.ActAt(Reputation::N, Reputation::N) == Action::C
    // ---
    && rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::G, Reputation::N, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::G, Action::D) == Reputation::B
    && rd.RepAt(Reputation::N, Reputation::N, Action::D) == Reputation::B
    // ---
    && ar.ActAt(Reputation::G, Reputation::B) == Action::D
    && ar.ActAt(Reputation::N, Reputation::B) == Action::D
    // ---------------
    && (rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::G, Reputation::B, Action::D) == Reputation::N)
    && (rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::G || rd.RepAt(Reputation::N, Reputation::B, Action::D) == Reputation::N)
    // ---
    && (rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::G || rd.RepAt(Reputation::B, Reputation::G, Action::C) == Reputation::N)
    && (rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::G || rd.RepAt(Reputation::B, Reputation::N, Action::D) == Reputation::N)
    // ---
    && ar.ActAt(Reputation::B, Reputation::G) == Action::C
    && ar.ActAt(Reputation::B, Reputation::N) == Action::D
    // ---
    && rd.RepAt(Reputation::B, Reputation::G, Action::D) == Reputation::B
    ) {
    return 13;
  }

  return 0;
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

  std::map<int, std::vector<uint64_t >> game_ids;
  HistoNormalBin h0_histo(0.05);
  HistoNormalBin h1_histo(0.05);
  HistoNormalBin h2_histo(0.05);

  while(fin) {
    uint64_t org_gid,gid;
    double c_prob,h0,h1,h2;
    fin >> org_gid >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      Game g(0.02, 0.02, gid, c_prob, {h0,h1,h2} );
      int i = ClassifyType(g);
      game_ids[i].push_back(gid);
    }
    h0_histo.Add(h0);
    h1_histo.Add(h1);
    h2_histo.Add(h2);
  }

  for (auto p : game_ids) {
    std::cerr << "type: " << p.first << ", " << p.second.size() << std::endl;
  }

  std::ofstream fout("non_L8.txt");
  for (auto p : game_ids) {
    if (p.first > 0) continue;
    for (uint64_t gid : p.second) {
      fout << p.first << ' ' << gid << std::endl;
    }
  }
  fout.close();

  /*
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
      if ( rd.RepAt(Reputation::G, Reputation::G, Action::D) == Reputation::N ) {
        rd = rd.Permute({1,0,2});
      }
      for (size_t n = 0; n < 18; n++) {
        histo[n][static_cast<int>(rd.reputations[n])]++;
      }
    }
    for (size_t n = 0; n < 18; n++) {
      Reputation rep_d = static_cast<Reputation>(n/6);
      Reputation rep_r = static_cast<Reputation>((n/2) % 3);
      Action act = static_cast<Action>(n % 2);
      std::ostringstream ss;
      std::cout << "(" << rep_d << "->" << rep_r << "," << act << ") : " << std::setw(8)
                << histo[n][0] << std::setw(8) << histo[n][1] << std::setw(8) << histo[n][2] << std::endl;
    }
  }
   */

  /*
  {
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
      std::cout << n << " : " << histo[n][0] << " " << histo[n][1] << std::endl;
    }
  }
   */

  /*
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
   */

  return 0;
}