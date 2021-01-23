#ifndef HISTO_NORMAL_BIN_HPP
#define HISTO_NORMAL_BIN_HPP

#include <iostream>
#include <map>


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

#endif // HISTO_NORMAL_BIN_HPP