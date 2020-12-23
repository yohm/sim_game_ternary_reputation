#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>

#include "ReputationDynamics.hpp"
#include "Game.hpp"


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

  std::vector<uint64_t> game_ids;

  while(fin) {
    uint64_t i;
    fin >> i;
    if (fin) { game_ids.push_back(i); }
  }

  std::cerr << "Loaded " << game_ids.size() << " games" << std::endl;
  for (uint64_t i : game_ids) {
    std::cout << i << std::endl;
  }

  return 0;
}