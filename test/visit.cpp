#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

// a variant that contains all types used in this file
using SomeType = std::variant<int, Point, Bbox, std::optional<Point>, std::vector<Point>, std::variant<int, Point>>;

namespace {

// Gather all the objects in the order visited
template <typename T>
std::vector<SomeType> gather_objects(const T& t) {
  std::vector<SomeType> values;
  knot::preorder(t, [&](const auto& value) { values.emplace_back(value); });
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

TEST(Visit, stop_searching) {
  const std::vector<std::tuple<int>> tuples{{1}, {2}, {3}};

  int visited = 0;
  knot::visit(tuples, [&](const auto& t) {
    visited++;
    // Should never visit ints
    const bool is_int = std::is_same_v<int, std::decay_t<decltype(t)>>;
    EXPECT_FALSE(is_int);
    // Don't visit through tuple<int>
    return !std::is_same_v<std::tuple<int>, std::decay_t<decltype(t)>>;
  });

  // Only visited the vector and its 3 tuples
  EXPECT_EQ(4, visited);
}

TEST(Visit, BigObject) {
  const Bbox small_box{Point{0, 0}, Point{1, 1}};
  const Bbox big_box{Point{0, 0}, Point{50, 50}};

  std::vector<Bbox> boxes;
  knot::visit(example_big_object(), [&](const auto& t) {
    if constexpr (std::is_same_v<Bbox, std::decay_t<decltype(t)>>) {
      boxes.push_back(t);
    }
  });

  const std::vector<Bbox> expected_boxes{small_box, big_box,   small_box, big_box,   small_box,
                                         big_box,   small_box, big_box,   small_box, big_box};
  EXPECT_EQ(expected_boxes, boxes);
}
