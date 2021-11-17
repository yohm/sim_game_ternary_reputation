#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <cassert>
#include <icecream.hpp>
#include "Strategy.hpp"
#include "Entry.hpp"


int main(int argc, char* argv[]) {

  if (argc < 2) {
    std::cerr << "wrong number of arguments" << std::endl;
    std::cerr << "  usage: " << argv[0] << " <ESS_ids_file> ..." << std::endl;
    throw std::runtime_error("wrong number of arguments");
  }

  for (int c = 1; c < argc; c++) {
    std::vector<Entry> v = Entry::LoadAndUniqSort(argv[c]);

    for (const Entry& e: v) {
      Strategy str(e.gid);
      if ( str.IsSecondOrder() ) {
        std::cout << str.Inspect() << std::endl;
      }
    }
  }

  return 0;
}