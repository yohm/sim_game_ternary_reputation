#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <icecream.hpp>

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

  using input_t = std::tuple<uint64_t,double,double,double,double>;

  std::vector<input_t> inputs;

  while(fin) {
    uint64_t gid;
    double c_prob,h0,h1,h2;
    fin >> gid >> c_prob >> h0 >> h1 >> h2;
    if (fin) {
      inputs.emplace_back(gid, c_prob, h0, h1, h2);
    }
  }

  std::sort(inputs.begin(), inputs.end(), [](const input_t& lhs, const input_t& rhs) {
    return (std::get<0>(lhs) < std::get<0>(rhs));
  });

  for(const auto& in: inputs) {
    std::cout << std::get<0>(in) << ' ' << std::get<1>(in) << ' ' << std::get<2>(in) << ' ' << std::get<3>(in) << ' ' << std::get<4>(in) << "\n";
  }

  return 0;
}