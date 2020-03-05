#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

// a variant that contains all types used in this file
using SomeType = std::variant<int, Point, Bbox, std::optional<Point>, std::vector<Point>, std::variant<int, Point>>;

namespace {

// gather all the objects I visit in the order visited
template <typename T>
std::vector<SomeType> gather_objects(const T& t) {
  std::vector<SomeType> values;
  knot::visit(t, [&](const auto& value) { values.emplace_back(value); });
  return values;
}

}  // namespace

TEST(Visit, primitive) { EXPECT_EQ(std::vector<SomeType>{5}, gather_objects(5)); }

TEST(Visit, basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p, p.x, p.y};
  EXPECT_EQ(expected, gather_objects(p));
}

TEST(Visit, composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox, bbox.min, bbox.min.x, bbox.min.y, bbox.max, bbox.max.x, bbox.max.y};
  EXPECT_EQ(expected, gather_objects(bbox));
}

TEST(Visit, optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{p, *p, p->x, p->y};
  EXPECT_EQ(expected, gather_objects(p));
}

TEST(Visit, nullopt) {
  EXPECT_EQ(std::vector<SomeType>{std::optional<Point>()}, gather_objects(std::optional<Point>()));
}

TEST(Visit, range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{vec, Point{1, 2}, 1, 2, Point{3, 4}, 3, 4};
  EXPECT_EQ(expected, gather_objects(vec));
}

TEST(Visit, empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{vec};
  EXPECT_EQ(expected, gather_objects(vec));
}

TEST(Visit, variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{var_point, Point{45, 89}, 45, 89};
  EXPECT_EQ(expected, gather_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{var_int, 5};
  EXPECT_EQ(expected2, gather_objects(var_int));
}
