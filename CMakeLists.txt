cmake_minimum_required(VERSION 3.15)

project(knot)

add_library(knot INTERFACE)
target_compile_features(knot INTERFACE cxx_std_17)
target_include_directories(knot INTERFACE include)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_subdirectory(test)
endif()
