#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <tuple>
#include <icecream.hpp>
#include "Entry.hpp"


int main(int argc, char* argv[]) {
  if (argc <= 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file1> <ESS_ids_file2> ...." << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<Entry> v = Entry::LoadAndUniqSort(argv[1]);

  for (int i = 2; i < argc; i++) {
    std::vector<Entry> v2 = Entry::LoadAndUniqSort(argv[i]);
    std::vector<Entry> result;
    std::set_intersection(v.begin(), v.end(), v2.begin(), v2.end(), std::inserter(result, result.end()));
    v = result;
  }

  for(const auto& x: v) {
    std::cout << x << "\n";
  }
  return 0;
}