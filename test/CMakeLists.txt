cmake_minimum_required(VERSION 3.1)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.cpp)

add_executable(knot_tests ${TEST_SOURCES})

target_compile_features(knot_tests PRIVATE cxx_std_17)
target_include_directories(knot_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/gtest/include)
target_link_libraries(knot_tests PUBLIC gtest)
