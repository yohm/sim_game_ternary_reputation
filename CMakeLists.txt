cmake_minimum_required(VERSION 3.7)
project(cpp)

set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(OpenMP REQUIRED)

set(SOURCE_FILES ReputationDynamics.hpp Game.hpp)

add_executable(main.out main.cpp ${SOURCE_FILES})
target_link_libraries(main.out PRIVATE OpenMP::OpenMP_CXX)

add_executable(test_ReputationDynamics.out test_ReputationDynamics.cpp ReputationDynamics.hpp)

add_executable(test_Game.out test_Game.cpp ReputationDynamics.hpp Game.hpp)

