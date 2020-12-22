#!/bin/bash -eux

mpiFCCpx -Nclang -std=c++14 -Kfast -Kopenmp -DNDEBUG -o main.out main.cpp

