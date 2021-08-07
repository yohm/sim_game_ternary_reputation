#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <icecream.hpp>


using input_t = std::tuple<uint64_t,double,double,double,double>;

void LoadFile(const char* fname, std::vector<input_t>& inputs) {
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
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file1> <ESS_ids_file2>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<input_t> inputs1, inputs2;
  LoadFile(argv[1], inputs1);
  LoadFile(argv[2], inputs2);

  std::sort(inputs1.begin(), inputs1.end(), [](const input_t& lhs, const input_t& rhs) {
    return (std::get<0>(lhs) < std::get<0>(rhs));
  });
  std::sort(inputs2.begin(), inputs2.end(), [](const input_t& lhs, const input_t& rhs) {
    return (std::get<0>(lhs) < std::get<0>(rhs));
  });

  std::vector<input_t > left_only, right_only;
  std::set_difference(inputs1.begin(), inputs1.end(), inputs2.begin(), inputs2.end(), std::inserter(left_only, left_only.end()),
                      [](const input_t& lhs, const input_t& rhs) { return (std::get<0>(lhs) < std::get<0>(rhs)); });
  std::set_difference(inputs2.begin(), inputs2.end(), inputs1.begin(), inputs1.end(), std::inserter(right_only, right_only.end()),
                      [](const input_t& lhs, const input_t& rhs) { return (std::get<0>(lhs) < std::get<0>(rhs)); });

  for(const auto& in: left_only) {
    std::cout << "< " << std::get<0>(in) << ' ' << std::get<1>(in) << ' ' << std::get<2>(in) << ' ' << std::get<3>(in) << ' ' << std::get<4>(in) << "\n";
  }
  for(const auto& in: right_only) {
    std::cout << "> " << std::get<0>(in) << ' ' << std::get<1>(in) << ' ' << std::get<2>(in) << ' ' << std::get<3>(in) << ' ' << std::get<4>(in) << "\n";
  }
  std::cerr << "size: < " << left_only.size() << ", > " << right_only.size() << "\n";

  return 0;
}