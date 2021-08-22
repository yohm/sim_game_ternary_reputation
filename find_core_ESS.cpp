#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <tuple>
#include <icecream.hpp>


class input_t {
  public:
  input_t(uint64_t _gid, double _c_prob, double _h0, double _h1, double _h2, double _b_lower, double _b_upper)
    : gid( _gid), c_prob( _c_prob), h({_h0, _h1, _h2}), b_range( {_b_lower, _b_upper}) {};
  uint64_t gid;
  double c_prob;
  std::array<double, 3> h;
  std::array<double, 2> b_range;

  void Print(std::ostream &out) const {
    out << gid << ' ' << c_prob << ' ' << h[0] << ' ' << h[1] << ' ' << h[2] << ' ' << b_range[0] << ' ' << b_range[1] << "\n";
  }
};

void LoadFile(const char* fname, std::vector<input_t>& inputs) {
  std::ifstream fin(fname);
  if (!fin) {
    std::cerr << "Failed to open file " << fname << std::endl;
    throw std::runtime_error("failed to open file");
  }

  while(fin) {
    uint64_t gid;
    double c_prob,h0,h1,h2,b_lower,b_upper;
    fin >> gid >> c_prob >> h0 >> h1 >> h2 >> b_lower >> b_upper;
    if (fin) {
      inputs.emplace_back( input_t(gid, c_prob, h0, h1, h2, b_lower, b_upper) );
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<input_t> inputs;
  LoadFile(argv[1], inputs);
  std::cerr << "loading " << argv[1] << std::endl;
  std::cerr << "  ESS size: " << inputs.size() << std::endl;

  std::vector<size_t> ess_count(101, 0ul);
  std::vector<input_t> core_set;
  // count for b is stored in ess_count[b/10]
  for (const auto& in: inputs) {
    size_t bi = std::ceil( in.b_range[0] * 10.0 );
    if (in.b_range[0] < 1.1) { bi = 11; }
    size_t bj = std::floor( in.b_range[1] * 10.0 );
    if (in.b_range[1] > 10.0) { bj = 100; }
    for (size_t b = bi; b <= bj; b++) {
      ess_count[b]++;
    }
    if (in.b_range[0] <= 1.1 && in.b_range[1] >= 10.0) {
      core_set.emplace_back(in);
    }
  }

  std::ofstream fout("core_ESS_ids");
  for(const auto& x: core_set) {
    x.Print(fout);
  }

  for (size_t b = 11; b <= 100; b++) {
    std::cout << b / 10.0 << ' ' << ess_count[b] << std::endl;
  }

  return 0;
}