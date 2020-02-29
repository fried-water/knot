cmake_minimum_required(VERSION 3.1)

add_executable(knot_tests test.cpp)

target_compile_features(knot_tests PRIVATE cxx_std_17)
target_include_directories(knot_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
