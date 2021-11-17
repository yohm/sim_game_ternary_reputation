cmake_minimum_required(VERSION 3.7)
project(sim_game_ternary_reputation_model)

set(CMAKE_CXX_STANDARD 14)

#set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/icecream /usr/local/include/eigen3 ${CMAKE_SOURCE_DIR}/json/include ${CMAKE_SOURCE_DIR}/caravan-lib)

set(SOURCE_FILES Strategy.hpp Game.hpp PopulationFlow.hpp)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
add_executable(print_normalized_RD.out print_normalized_RD.cpp Strategy.hpp)

add_executable(main_search_ESS.out main_search_ESS.cpp ${SOURCE_FILES})
target_link_libraries(main_search_ESS.out PRIVATE OpenMP::OpenMP_CXX ${MPI_LIBRARIES})

add_executable(main_classify_ESS.out main_classify_ESS.cpp ${SOURCE_FILES} HistoNormalBin.hpp)
target_link_libraries(main_classify_ESS.out PRIVATE OpenMP::OpenMP_CXX)

add_executable(find_second_order_norms.out find_second_order_norms.cpp ${SOURCE_FILES} HistoNormalBin.hpp Entry.hpp)

add_executable(sort_uniq_ESSs.out sort_uniq_ESS.cpp Entry.hpp)
add_executable(diff_ESS.out diff_ESS.cpp Entry.hpp)
add_executable(find_core_ESS.out find_core_ESS.cpp Entry.hpp)


add_executable(test_Strategy.out test_Strategy.cpp Strategy.hpp)
add_executable(test_Game.out test_Game.cpp Strategy.hpp Game.hpp)

add_executable(check_initial_condition.out check_initial_condition.cpp Game.hpp Strategy.hpp)
target_link_libraries(check_initial_condition.out PRIVATE OpenMP::OpenMP_CXX)
