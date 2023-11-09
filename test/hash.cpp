#include "knot/hash.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

#include <unordered_set>
#include <iostream>

namespace {

std::size_t hash_combine(std::size_t seed, std::size_t hash) {
  return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}  // namespace

BOOST_AUTO_TEST_CASE(hash_primitive) { BOOST_CHECK(std::hash<int>{}(5) == knot::hash_value(5)); }

BOOST_AUTO_TEST_CASE(hash_basic_struct) {
  const std::size_t expected_hash = hash_combine(hash_combine(0, 45), 89);
  BOOST_CHECK(expected_hash == knot::hash_value(Point{45, 89}));

  BOOST_CHECK(knot::hash_value(Point{1, 2}) != knot::hash_value(Point{2, 1}));
}

BOOST_AUTO_TEST_CASE(hash_composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::size_t expected_hash =
      hash_combine(hash_combine(0, knot::hash_value(Point{1, 2})), knot::hash_value(Point{3, 4}));
  BOOST_CHECK(expected_hash == knot::hash_value(bbox));
}

BOOST_AUTO_TEST_CASE(hash_basic_optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::size_t expected_hash =
      hash_combine(static_cast<std::size_t>(static_cast<bool>(true)), knot::hash_value(Point{45, 89}));
  BOOST_CHECK(expected_hash == knot::hash_value(p));

  BOOST_CHECK(0 == knot::hash_value(std::optional<Point>{}));
  BOOST_CHECK(0 == knot::hash_value(std::optional<int>{}));
}

BOOST_AUTO_TEST_CASE(hash_basic_unique_ptr) {
  const auto p = std::make_unique<Point>(Point{45, 89});
  BOOST_CHECK(knot::hash_value(std::optional(Point{45, 89})) == knot::hash_value(p));

  BOOST_CHECK(0 == knot::hash_value(std::unique_ptr<Point>()));
  BOOST_CHECK(0 != knot::hash_value(std::make_unique<int>(0)));
}

BOOST_AUTO_TEST_CASE(hash_basic_ptr) {
  const void* ptr = (const void*)0xdeafbeef;
  BOOST_CHECK(std::hash<const void*>{}(ptr) == knot::hash_value(ptr));
}

BOOST_AUTO_TEST_CASE(hash_basic_range) {
  const std::vector<int> vec{1, 2, 3};
  const std::size_t expected_hash = hash_combine(hash_combine(hash_combine(0, 1), 2), 3);
  BOOST_CHECK(expected_hash == knot::hash_value(vec));

  BOOST_CHECK(0 == knot::hash_value(std::vector<int>{}));

  // empty and single 0 value shouldnt hash to the same thing
  BOOST_CHECK(knot::hash_value(std::vector<int>{0}) != knot::hash_value(std::vector<int>{}));
}

BOOST_AUTO_TEST_CASE(hash_basic_variant) {
  const std::variant<int, Point> var = Point{45, 89};
  const std::size_t expected_hash = hash_combine(1, knot::hash_value(Point{45, 89}));
  BOOST_CHECK(expected_hash == knot::hash_value(var));

  // different 0 values shouldnt hash to the same thing
  BOOST_CHECK(knot::hash_value(std::variant<int, long>{0}) != knot::hash_value(std::variant<int, long>{0l}));
}

BOOST_AUTO_TEST_CASE(hash_unordered_containers) {
  // Can put knot::hash-able things in unordered_containers
  std::unordered_set<Point, knot::Hash> set1;
  set1.insert({});

  std::unordered_set<Bbox, knot::Hash> set2;
  set2.insert({});

  std::unordered_set<std::vector<Bbox>, knot::Hash> set3;
  set3.insert({});

  std::unordered_set<BigObject, knot::Hash> set4;
  set4.insert(BigObject{});
}

BOOST_AUTO_TEST_CASE(hash_non_tuple_tie) {
  BOOST_CHECK(knot::hash_value(5) == knot::hash_value(IntWrapper{5}));
  BOOST_CHECK(knot::hash_value(std::vector<int>{1, 2, 3}) == knot::hash_value(VecWrapper{{1, 2, 3}}));
  BOOST_CHECK(knot::hash_value(std::variant<int, float>(5.0f)) == knot::hash_value(VariantWrapper{5.0f}));
}
