#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <tuple>
#include <icecream.hpp>
#include "Entry.hpp"


int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file>" << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  std::vector<Entry> inputs = Entry::LoadAndUniqSort(argv[1]);
  for(const auto& in: inputs) {
    std::cout << in << "\n";
  }

  return 0;
}