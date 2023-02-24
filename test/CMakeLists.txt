cmake_minimum_required(VERSION 3.15)

find_package(Boost REQUIRED COMPONENTS unit_test_framework system)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.cpp)

add_executable(knot_test ${TEST_SOURCES})

target_compile_features(knot_test PRIVATE cxx_std_17)
target_link_libraries(knot_test PUBLIC knot ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

set_target_properties(knot_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_test(NAME KnotUnitTests COMMAND knot_tests)
