#!/bin/bash -eux

mpiFCCpx -Nclang -std=c++14 -Kfast -Kopenmp -DNDEBUG -Ijson/include -Iicecream -Icaravan-lib -o main_search_ESS.out main_search_ESS.cpp

