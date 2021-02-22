#!/bin/bash -eux

mpiFCCpx -Nclang -std=c++14 -Kfast -Kopenmp -DNDEBUG -o main_search_ESS.out main_search_ESS.cpp

