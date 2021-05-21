cmake_minimum_required(VERSION 3.7)
project(sim_game_ternary_reputation_model)

set(CMAKE_CXX_STANDARD 14)

#set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/icecream /usr/local/include/eigen3 ${CMAKE_SOURCE_DIR}/json/include ${CMAKE_SOURCE_DIR}/caravan-lib)

set(SOURCE_FILES Strategy.hpp Game.hpp PopulationFlow.hpp)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
add_executable(main_search_ESS.out main_search_ESS.cpp ${SOURCE_FILES})
target_link_libraries(main_search_ESS.out PRIVATE OpenMP::OpenMP_CXX ${MPI_LIBRARIES})

add_executable(calc_histo.out calc_histo.cpp ${SOURCE_FILES} HistoNormalBin.hpp)
target_link_libraries(calc_histo.out PRIVATE OpenMP::OpenMP_CXX)

add_executable(calc_flow.out calc_flow.cpp ${SOURCE_FILES} HistoNormalBin.hpp)
target_link_libraries(calc_flow.out PRIVATE OpenMP::OpenMP_CXX)

add_executable(sort_ESSs.out sort_ESSs.cpp)
add_executable(diff_ESSs.out diff_ESSs.cpp)

add_executable(print_normalized_RD.out print_normalized_RD.cpp Strategy.hpp)

add_executable(test_ReputationDynamics.out test_Strategy.cpp Strategy.hpp)

add_executable(test_Game.out test_Game.cpp Strategy.hpp Game.hpp)
