#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <icecream.hpp>
#include "Entry.hpp"


int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file1> <ESS_ids_file2>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<Entry> inputs1 = Entry::LoadAndUniqSort(argv[1]), inputs2 = Entry::LoadAndUniqSort(argv[2]);

  std::vector<Entry> left_only, right_only;

  std::set_difference(inputs1.begin(), inputs1.end(), inputs2.begin(), inputs2.end(), std::inserter(left_only, left_only.end()));
  std::set_difference(inputs2.begin(), inputs2.end(), inputs1.begin(), inputs1.end(), std::inserter(right_only, right_only.end()));

  for(const auto& in: left_only) {
    std::cout << "< " << in << "\n";
  }
  for(const auto& in: right_only) {
    std::cout << "> " << in << "\n";
  }
  std::cerr << "size: < " << left_only.size() << ", > " << right_only.size() << "\n";

  return 0;
}