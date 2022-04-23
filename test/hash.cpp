#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

#include <unordered_set>

namespace {

std::size_t hash_combine(std::size_t seed, std::size_t hash) {
  return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

}  // namespace

TEST(Hash, primitive) {
  EXPECT_EQ(std::hash<int>{}(5), knot::hash_value(5));
}

TEST(Hash, basic_struct) {
  const std::size_t expected_hash = hash_combine(hash_combine(0, 45), 89);
  EXPECT_EQ(expected_hash, knot::hash_value(Point{45, 89}));

  EXPECT_NE(knot::hash_value(Point{1, 2}), knot::hash_value(Point{2, 1}));
}

TEST(Hash, composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::size_t expected_hash =  hash_combine(hash_combine(0, knot::hash_value(Point{1, 2})), knot::hash_value(Point{3, 4}));
  EXPECT_EQ(expected_hash, knot::hash_value(bbox));
}

TEST(Hash, basic_optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::size_t expected_hash = hash_combine(static_cast<std::size_t>(static_cast<bool>(true)), knot::hash_value(Point{45, 89}));
  EXPECT_EQ(expected_hash, knot::hash_value(p));

  EXPECT_EQ(0, knot::hash_value(std::optional<Point>{}));

  // This falls back on std hash
  EXPECT_EQ(0, knot::hash_value(std::optional<int>{0}));
}

TEST(Hash, basic_unique_ptr) {
  const auto p = std::make_unique<Point>(Point{45, 89});
  const std::size_t expected_hash = std::hash<std::unique_ptr<Point>>{}(p);
  EXPECT_EQ(expected_hash, knot::hash_value(p));

  // These fall back on std hash
  EXPECT_NE(0, knot::hash_value(std::unique_ptr<Point>()));
  EXPECT_NE(0, knot::hash_value(std::make_unique<int>(0)));
}

TEST(Hash, basic_range) {
  const std::vector<int> vec{1, 2, 3};
  const std::size_t expected_hash = hash_combine(hash_combine(hash_combine(0, 1), 2), 3);
  EXPECT_EQ(expected_hash, knot::hash_value(vec));

  EXPECT_EQ(0, knot::hash_value(std::vector<int>{}));

  // empty and single 0 value shouldnt hash to the same thing
  EXPECT_NE(knot::hash_value(std::vector<int>{0}), knot::hash_value(std::vector<int>{}));
}

TEST(Hash, basic_variant) {
  const std::variant<int, Point> var = Point{45, 89};
  const std::size_t expected_hash = hash_combine(1, knot::hash_value(Point{45, 89}));
  EXPECT_EQ(expected_hash, knot::hash_value(var));

  // different 0 values shouldnt hash to the same thing
  EXPECT_NE(knot::hash_value(std::variant<int, long>{0}), knot::hash_value(std::variant<int, long>{0l}));
}

TEST(Hash, unordered_containers) {
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

TEST(Hash, non_tuple_tie) {
  EXPECT_EQ(knot::hash_value(5), knot::hash_value(IntWrapper{5}));
  EXPECT_EQ(knot::hash_value(std::vector<int>{1, 2, 3}), knot::hash_value(VecWrapper{{1, 2, 3}}));
  EXPECT_EQ(knot::hash_value(std::variant<int, float>(5.0f)), knot::hash_value(VariantWrapper{5.0f}));
}
