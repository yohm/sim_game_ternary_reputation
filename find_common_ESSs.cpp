#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <icecream.hpp>


using input_t = std::tuple<uint64_t,double,double,double,double>;

void LoadFileAndSort(const char* fname, std::vector<input_t>& inputs) {
  std::ifstream fin(fname);
  if (!fin) {
    std::cerr << "Failed to open file " << fname << std::endl;
    throw std::runtime_error("failed to open file");
  }

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
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file1> <ESS_ids_file2> ..." << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<input_t> v_union;
  LoadFileAndSort(argv[1], v_union);
  std::cerr << "loading " << argv[1] << std::endl;
  std::cerr << "  common size: " << v_union.size() << std::endl;

  for (int i = 2; i < argc; i++) {
    std::vector<input_t> inputs;
    std::vector<input_t> org(v_union.size());
    std::copy(v_union.begin(), v_union.end(), org.begin());
    v_union.clear();
    std::cerr << "loading " << argv[i] << std::endl;
    LoadFileAndSort(argv[i], inputs);
    std::set_intersection(org.begin(), org.end(), inputs.begin(), inputs.end(), std::inserter(v_union, v_union.end()),
                   [](const input_t& lhs, const input_t& rhs) { return (std::get<0>(lhs) < std::get<0>(rhs)); });
    std::cerr << "  common size: " << v_union.size() << std::endl;
  }

  for(const auto& in: v_union) {
    std::cout << std::get<0>(in) << ' ' << std::get<1>(in) << ' ' << std::get<2>(in) << ' ' << std::get<3>(in) << ' ' << std::get<4>(in) << "\n";
  }

  return 0;
}