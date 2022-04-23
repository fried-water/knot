cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.cpp)

add_executable(knot_tests ${TEST_SOURCES})

target_compile_features(knot_tests PRIVATE cxx_std_17)
target_link_libraries(knot_tests PUBLIC knot gtest)

include(GoogleTest)
gtest_discover_tests(knot_tests)
