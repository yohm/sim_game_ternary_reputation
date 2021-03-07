#!/bin/bash -eux

mpiFCCpx -Nclang -std=c++14 -Kfast -Kopenmp -DNDEBUG -I$HOME/sandbox/eigen-3.3.7 -Ijson/include -Iicecream -Icaravan-lib -o main_search_ESS.out main_search_ESS.cpp
mpiFCCpx -Nclang -std=c++14 -Kfast -Kopenmp -DNDEBUG -I$HOME/sandbox/eigen-3.3.7 -Ijson/include -Iicecream -Icaravan-lib -o main_private_rep_coop_prob.out main_private_rep_coop_prob.cpp

