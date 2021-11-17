#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <tuple>
#include <icecream.hpp>


struct Entry {
  Entry(uint64_t _gid, double _c_prob, double h0, double h1, double h2) : gid(_gid), c_prob(_c_prob), h({h0,h1,h2}) {};
  uint64_t gid;
  double c_prob;
  std::array<double,3> h;
  friend std::ostream& operator<<(std::ostream& os, const Entry& e) {
    os << e.gid << ' ' << e.c_prob << ' ' << e.h[0] << ' ' << e.h[1] << ' ' << e.h[2];
    return os;
  };
  static std::vector<Entry> Load(std::ifstream& fin) {
    std::vector<Entry> v;
    while (fin) {
      uint64_t gid;
      double c_prob, h0, h1, h2;
      fin >> gid >> c_prob >> h0 >> h1 >> h2;
      v.emplace_back(gid, c_prob, h0, h1, h2);
    }
    return v;
  }
  bool operator<(const Entry& rhs) const {
    return gid < rhs.gid;
  }
  bool operator==(const Entry& rhs) const {
    return gid == rhs.gid;
  }
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

  std::vector<Entry> inputs = Entry::Load(fin);

  std::sort(inputs.begin(), inputs.end());
  auto uniq_begin = std::unique(inputs.begin(), inputs.end());
  inputs.erase(uniq_begin, inputs.end());

  for(const auto& in: inputs) {
    std::cout << in << "\n";
  }

  return 0;
}