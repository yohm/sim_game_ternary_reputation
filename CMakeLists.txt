cmake_minimum_required(VERSION 3.7)
project(cpp)

set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_VERBOSE_MAKEFILE 1)

set(SOURCE_FILES ReputationDynamics.hpp Game.hpp)
find_package(MPI)

add_executable(main.out main.cpp ${SOURCE_FILES})

