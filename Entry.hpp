#ifndef ENTRY_HPP
#define ENTRY_HPP


#include <iostream>
#include <fstream>
#include <vector>
#include <array>

struct Entry {
  Entry(uint64_t _gid, double _c_prob, double h0, double h1, double h2) : gid(_gid), c_prob(_c_prob), h({h0,h1,h2}) {};
  uint64_t gid;
  double c_prob;
  std::array<double,3> h;
  friend std::ostream& operator<<(std::ostream& os, const Entry& e) {
    os << e.gid << ' ' << e.c_prob << ' ' << e.h[0] << ' ' << e.h[1] << ' ' << e.h[2];
    return os;
  };
  bool operator<(const Entry& rhs) const {
    return gid < rhs.gid;
  }
  bool operator==(const Entry& rhs) const {
    return gid == rhs.gid;
  }
  static std::vector<Entry> Load(const char* fname) {
    std::ifstream fin(fname);
    if (!fin) {
      std::cerr << "Failed to open file: " << fname << std::endl;
      throw std::runtime_error("failed to open file");
    }
    std::vector<Entry> v;
    while (fin) {
      uint64_t gid;
      double c_prob, h0, h1, h2;
      fin >> gid >> c_prob >> h0 >> h1 >> h2;
      v.emplace_back(gid, c_prob, h0, h1, h2);
    }
    return v;
  }
  static std::vector<Entry> LoadAndUniqSort(const char* fname) {
    auto v = Load(fname);
    std::sort(v.begin(), v.end());
    auto it = std::unique(v.begin(), v.end());
    v.erase(it, v.end());
    return v;
  }
};

#endif  // ENTRY_HPP
