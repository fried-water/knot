#include "test_structs.h"

#include "knot/core.h"

#include "gtest/gtest.h"

// a variant that contains all types used in this file
using SomeType = std::variant<int, Point, Bbox, std::optional<Point>, std::vector<Point>, std::variant<int, Point>,
  IntWrapper, VecWrapper, std::vector<int>, VariantWrapper, std::variant<int, float>>;

namespace {

// Gather all the objects in the order visited
template <typename T>
std::vector<SomeType> gather_objects(const T& t) {
  std::vector<SomeType> values;
  knot::visit(t, [&](const auto& value) { values.emplace_back(value); });
  return values;
}

// Gather all the objects during a preorder traversal in the order visited
template <typename T>
std::vector<SomeType> gather_preorder_objects(const T& t) {
  std::vector<SomeType> values;
  knot::preorder(t, [&](const auto& value) { values.emplace_back(value); });
  return values;
}

// Gather all the objects during a postorder traversal in the order visited
template <typename T>
std::vector<SomeType> gather_postorder_objects(const T& t) {
  std::vector<SomeType> values;
  knot::postorder(t, [&](const auto& value) { values.emplace_back(value); });
  return values;
}

}  // namespace

TEST(Visit, primitive) { EXPECT_EQ(std::vector<SomeType>{}, gather_objects(5)); }

TEST(Visit, basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p.x, p.y};
  EXPECT_EQ(expected, gather_objects(p));
}

TEST(Visit, composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox.min, bbox.max};
  EXPECT_EQ(expected, gather_objects(bbox));
}

TEST(Visit, optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{*p};
  EXPECT_EQ(expected, gather_objects(p));
}

TEST(Visit, nullopt) {
  EXPECT_EQ(std::vector<SomeType>{}, gather_objects(std::optional<Point>()));
}

TEST(Visit, range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{Point{1, 2}, Point{3, 4}};
  EXPECT_EQ(expected, gather_objects(vec));
}

TEST(Visit, empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{};
  EXPECT_EQ(expected, gather_objects(vec));
}

TEST(Visit, variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{Point{45, 89}};
  EXPECT_EQ(expected, gather_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{5};
  EXPECT_EQ(expected2, gather_objects(var_int));
}

TEST(Visit, non_tuple_tie) {
  EXPECT_EQ(gather_objects(5), gather_objects(IntWrapper{5}));
  EXPECT_EQ(gather_objects(std::vector<int>{5}), gather_objects(VecWrapper{{5}}));
  EXPECT_EQ(gather_objects(std::variant<int, float>(5)), gather_objects(VariantWrapper{5}));
}

TEST(Preorder, primitive) { EXPECT_EQ(std::vector<SomeType>{5}, gather_preorder_objects(5)); }

TEST(Preorder, basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p, p.x, p.y};
  EXPECT_EQ(expected, gather_preorder_objects(p));
}

TEST(Preorder, composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox, bbox.min, bbox.min.x, bbox.min.y, bbox.max, bbox.max.x, bbox.max.y};
  EXPECT_EQ(expected, gather_preorder_objects(bbox));
}

TEST(Preorder, optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{p, *p, p->x, p->y};
  EXPECT_EQ(expected, gather_preorder_objects(p));
}

TEST(Preorder, nullopt) {
  EXPECT_EQ(std::vector<SomeType>{std::optional<Point>()}, gather_preorder_objects(std::optional<Point>()));
}

TEST(Preorder, range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{vec, Point{1, 2}, 1, 2, Point{3, 4}, 3, 4};
  EXPECT_EQ(expected, gather_preorder_objects(vec));
}

TEST(Preorder, empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{vec};
  EXPECT_EQ(expected, gather_preorder_objects(vec));
}

TEST(Preorder, variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{var_point, Point{45, 89}, 45, 89};
  EXPECT_EQ(expected, gather_preorder_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{var_int, 5};
  EXPECT_EQ(expected2, gather_preorder_objects(var_int));
}

TEST(Preorder, stop_searching) {
  const std::vector<std::tuple<int>> tuples{{1}, {2}, {3}};

  int visited = 0;
  knot::preorder(tuples, [&](const auto& t) {
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

TEST(Preorder, BigObject) {
  const Bbox small_box{Point{0, 0}, Point{1, 1}};
  const Bbox big_box{Point{0, 0}, Point{50, 50}};

  std::vector<Bbox> boxes;
  knot::preorder(example_big_object(), [&](const Bbox& t) { boxes.push_back(t); });

  const std::vector<Bbox> expected_boxes{small_box, big_box,   small_box, big_box,   small_box,
                                         big_box,   small_box, big_box,   small_box, big_box};
  EXPECT_EQ(expected_boxes, boxes);
}

TEST(Preorder, non_tuple_tie) {
  EXPECT_EQ((std::vector<SomeType>{IntWrapper{5}}), gather_preorder_objects(IntWrapper{5}));
  EXPECT_EQ((std::vector<SomeType>{VecWrapper{{5}}, 5}), gather_preorder_objects(VecWrapper{{5}}));
  EXPECT_EQ((std::vector<SomeType>{VariantWrapper{5}, 5}), gather_preorder_objects(VariantWrapper{5}));
}

TEST(Postorder, primitive) { EXPECT_EQ(std::vector<SomeType>{5}, gather_postorder_objects(5)); }

TEST(Postorder, basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p.x, p.y, p};
  EXPECT_EQ(expected, gather_postorder_objects(p));
}

TEST(Postorder, composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox.min.x, bbox.min.y, bbox.min, bbox.max.x, bbox.max.y, bbox.max, bbox};
  EXPECT_EQ(expected, gather_postorder_objects(bbox));
}

TEST(Postorder, optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{p->x, p->y, *p, p};
  EXPECT_EQ(expected, gather_postorder_objects(p));
}

TEST(Postorder, nullopt) {
  EXPECT_EQ(std::vector<SomeType>{std::optional<Point>()}, gather_postorder_objects(std::optional<Point>()));
}

TEST(Postorder, range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{1, 2, Point{1, 2}, 3, 4, Point{3, 4}, vec};
  EXPECT_EQ(expected, gather_postorder_objects(vec));
}

TEST(Postorder, empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{vec};
  EXPECT_EQ(expected, gather_postorder_objects(vec));
}

TEST(Postorder, variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{45, 89, Point{45, 89}, var_point};
  EXPECT_EQ(expected, gather_postorder_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{5, var_int};
  EXPECT_EQ(expected2, gather_postorder_objects(var_int));
}

TEST(Postorder, non_tuple_tie) {
  EXPECT_EQ((std::vector<SomeType>{IntWrapper{5}}), gather_postorder_objects(IntWrapper{5}));
  EXPECT_EQ((std::vector<SomeType>{5, VecWrapper{{5}}}), gather_postorder_objects(VecWrapper{{5}}));
  EXPECT_EQ((std::vector<SomeType>{5, VariantWrapper{5}}), gather_postorder_objects(VariantWrapper{5}));
}
