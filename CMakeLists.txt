cmake_minimum_required(VERSION 3.1)

project(knot)

add_library(knot INTERFACE)
target_compile_features(knot INTERFACE cxx_std_17)

add_subdirectory(test)
add_subdirectory(lib/googletest)

enable_testing()
add_test (NAME test COMMAND test/knot_tests)
