#include "knot/traversals.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_CASE(visit_primitive) { BOOST_CHECK(std::vector<SomeType>{} == gather_objects(5)); }

BOOST_AUTO_TEST_CASE(visit_basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p.x, p.y};
  BOOST_CHECK(expected == gather_objects(p));
}

BOOST_AUTO_TEST_CASE(visit_composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox.min, bbox.max};
  BOOST_CHECK(expected == gather_objects(bbox));
}

BOOST_AUTO_TEST_CASE(visit_optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{*p};
  BOOST_CHECK(expected == gather_objects(p));
}

BOOST_AUTO_TEST_CASE(visit_nullopt) { BOOST_CHECK(std::vector<SomeType>{} == gather_objects(std::optional<Point>())); }

BOOST_AUTO_TEST_CASE(visit_range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{Point{1, 2}, Point{3, 4}};
  BOOST_CHECK(expected == gather_objects(vec));
}

BOOST_AUTO_TEST_CASE(visit_empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{};
  BOOST_CHECK(expected == gather_objects(vec));
}

BOOST_AUTO_TEST_CASE(visit_variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{Point{45, 89}};
  BOOST_CHECK(expected == gather_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{5};
  BOOST_CHECK(expected2 == gather_objects(var_int));
}

BOOST_AUTO_TEST_CASE(visit_non_tuple_tie) {
  BOOST_CHECK(gather_objects(5) == gather_objects(IntWrapper{5}));
  BOOST_CHECK(gather_objects(std::vector<int>{5}) == gather_objects(VecWrapper{{5}}));
  BOOST_CHECK(gather_objects(std::variant<int, float>(5)) == gather_objects(VariantWrapper{5}));
}

BOOST_AUTO_TEST_CASE(visit_primitive_lvalue) {
  int x = 7;
  knot::visit(x, [](int&) { BOOST_CHECK(false); });
  BOOST_CHECK(7 == x);
}

BOOST_AUTO_TEST_CASE(visit_struct_lvalue) {
  Point p{0, 0};
  knot::visit(p, [](int& y) { y = 1; });
  BOOST_CHECK((Point{1, 1}) == p);
}

BOOST_AUTO_TEST_CASE(visit_optional_lvalue) {
  std::optional<Point> p = Point{0, 0};
  knot::visit(p, [](Point& y) { y = {1, 2}; });
  BOOST_CHECK((Point{1, 2}) == p);
}

BOOST_AUTO_TEST_CASE(visit_variant_lvalue) {
  std::variant<int, Point> v = 0;
  knot::visit(v, [](int& y) { y = 2; });
  BOOST_CHECK((std::variant<int, Point>{2}) == v);
}

BOOST_AUTO_TEST_CASE(visit_range_lvalue) {
  std::vector<int> v{1, 2, 3};
  knot::visit(v, [](int& y) { y = 7; });
  BOOST_CHECK((std::vector<int>{7, 7, 7}) == v);
}

BOOST_AUTO_TEST_CASE(visit_move_only) {
  const auto extract_unique_ptr = [](auto t) {
    std::unique_ptr<int> ptr;
    knot::visit(std::move(t), [&](std::unique_ptr<int> p) { ptr = std::move(p); });
    return ptr ? std::optional(*ptr) : std::nullopt;
  };

  BOOST_CHECK(std::nullopt == extract_unique_ptr(std::make_unique<int>(7)));

  BOOST_CHECK(std::nullopt == extract_unique_ptr(5));
  BOOST_CHECK(std::nullopt == extract_unique_ptr(std::pair('a', 5)));
  BOOST_CHECK(7 == extract_unique_ptr(std::pair{5, std::make_unique<int>(7)}));
  BOOST_CHECK(7 == extract_unique_ptr(std::variant<int, std::unique_ptr<int>>{std::make_unique<int>(7)}));

  std::vector<std::unique_ptr<int>> v;
  v.push_back(std::make_unique<int>(1));
  v.push_back(std::make_unique<int>(2));
  BOOST_CHECK(2 == extract_unique_ptr(std::move(v)));
  BOOST_CHECK(std::nullopt == extract_unique_ptr(std::move(v)));
}

BOOST_AUTO_TEST_CASE(preorder_primitive) { BOOST_CHECK(std::vector<SomeType>{5} == gather_preorder_objects(5)); }

BOOST_AUTO_TEST_CASE(preorder_basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p, p.x, p.y};
  BOOST_CHECK(expected == gather_preorder_objects(p));
}

BOOST_AUTO_TEST_CASE(preorder_composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox, bbox.min, bbox.min.x, bbox.min.y, bbox.max, bbox.max.x, bbox.max.y};
  BOOST_CHECK(expected == gather_preorder_objects(bbox));
}

BOOST_AUTO_TEST_CASE(preorder_optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{p, *p, p->x, p->y};
  BOOST_CHECK(expected == gather_preorder_objects(p));
}

BOOST_AUTO_TEST_CASE(preorder_nullopt) {
  BOOST_CHECK(std::vector<SomeType>{std::optional<Point>()} == gather_preorder_objects(std::optional<Point>()));
}

BOOST_AUTO_TEST_CASE(preorder_range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{vec, Point{1, 2}, 1, 2, Point{3, 4}, 3, 4};
  BOOST_CHECK(expected == gather_preorder_objects(vec));
}

BOOST_AUTO_TEST_CASE(preorder_empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{vec};
  BOOST_CHECK(expected == gather_preorder_objects(vec));
}

BOOST_AUTO_TEST_CASE(preorder_variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{var_point, Point{45, 89}, 45, 89};
  BOOST_CHECK(expected == gather_preorder_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{var_int, 5};
  BOOST_CHECK(expected2 == gather_preorder_objects(var_int));
}

BOOST_AUTO_TEST_CASE(preorder_stop_searching) {
  const std::vector<std::tuple<int>> tuples{{1}, {2}, {3}};

  int visited = 0;
  knot::preorder(tuples, [&](const auto& t) {
    visited++;
    // Should never visit ints
    const bool is_int = std::is_same_v<int, std::decay_t<decltype(t)>>;
    BOOST_TEST(!is_int);
    // Don't visit through tuple<int>
    return !std::is_same_v<std::tuple<int>, std::decay_t<decltype(t)>>;
  });

  // Only visited the vector and its 3 tuples
  BOOST_CHECK(4 == visited);
}

BOOST_AUTO_TEST_CASE(preorder_BigObject) {
  const Bbox small_box{Point{0, 0}, Point{1, 1}};
  const Bbox big_box{Point{0, 0}, Point{50, 50}};

  std::vector<Bbox> boxes;
  knot::preorder(example_big_object(), [&](const Bbox& t) { boxes.push_back(t); });

  const std::vector<Bbox> expected_boxes{small_box, big_box,   small_box, big_box,   small_box,
                                         big_box,   small_box, big_box,   small_box, big_box};
  BOOST_CHECK(expected_boxes == boxes);
}

BOOST_AUTO_TEST_CASE(preorder_non_tuple_tie) {
  BOOST_CHECK((std::vector<SomeType>{IntWrapper{5}}) == gather_preorder_objects(IntWrapper{5}));
  BOOST_CHECK((std::vector<SomeType>{VecWrapper{{5}}, 5}) == gather_preorder_objects(VecWrapper{{5}}));
  BOOST_CHECK((std::vector<SomeType>{VariantWrapper{5}, 5}) == gather_preorder_objects(VariantWrapper{5}));
}

BOOST_AUTO_TEST_CASE(accumulate_move) {
  std::vector<std::unique_ptr<int>> in;
  in.push_back(std::make_unique<int>(0));
  in.push_back(std::make_unique<int>(1));

  auto obj = std::tuple(std::move(in), std::variant<int, std::unique_ptr<int>>{std::make_unique<int>(2)},
                        std::optional(std::make_unique<int>(3)), std::make_unique<int>(4));

  const auto v =
      knot::accumulate(std::move(obj), std::vector<std::unique_ptr<int>>{}, [](auto v, std::unique_ptr<int> ptr) {
        v.push_back(std::move(ptr));
        return v;
      });

  BOOST_REQUIRE(1 == v.size());
  BOOST_CHECK(v.front() && 4 == *v.front());
}

BOOST_AUTO_TEST_CASE(preorder_move) {
  std::vector<std::unique_ptr<int>> in;
  in.push_back(std::make_unique<int>(0));
  in.push_back(std::make_unique<int>(1));

  auto obj = std::tuple(std::move(in), std::variant<int, std::unique_ptr<int>>{std::make_unique<int>(2)},
                        std::optional(std::make_unique<int>(3)), std::make_unique<int>(4));

  std::vector<std::unique_ptr<int>> v;
  knot::preorder(std::move(obj), [&](std::unique_ptr<int> ptr) { v.push_back(std::move(ptr)); });

  BOOST_REQUIRE(5 == v.size());
  for (int i = 0; i < 5; i++) {
    BOOST_CHECK(v[i] && *v[i] == i);
  }
}

BOOST_AUTO_TEST_CASE(preorder_accumulate_move) {
  std::vector<std::unique_ptr<int>> in;
  in.push_back(std::make_unique<int>(0));
  in.push_back(std::make_unique<int>(1));

  auto obj = std::tuple(std::move(in), std::variant<int, std::unique_ptr<int>>{std::make_unique<int>(2)},
                        std::optional(std::make_unique<int>(3)), std::make_unique<int>(4));

  const auto v = knot::preorder_accumulate(std::move(obj), std::vector<std::unique_ptr<int>>{},
                                           [](auto v, std::unique_ptr<int> ptr) {
                                             v.push_back(std::move(ptr));
                                             return v;
                                           });

  BOOST_REQUIRE(5 == v.size());
  for (int i = 0; i < 5; i++) {
    BOOST_CHECK(v[i] && *v[i] == i);
  }
}

BOOST_AUTO_TEST_CASE(postorder_primitive) { BOOST_CHECK(std::vector<SomeType>{5} == gather_postorder_objects(5)); }

BOOST_AUTO_TEST_CASE(postorder_basic_struct) {
  const Point p{45, 89};
  const std::vector<SomeType> expected{p.x, p.y, p};
  BOOST_CHECK(expected == gather_postorder_objects(p));
}

BOOST_AUTO_TEST_CASE(postorder_composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{bbox.min.x, bbox.min.y, bbox.min, bbox.max.x, bbox.max.y, bbox.max, bbox};
  BOOST_CHECK(expected == gather_postorder_objects(bbox));
}

BOOST_AUTO_TEST_CASE(postorder_optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<SomeType> expected{p->x, p->y, *p, p};
  BOOST_CHECK(expected == gather_postorder_objects(p));
}

BOOST_AUTO_TEST_CASE(postorder_nullopt) {
  BOOST_CHECK(std::vector<SomeType>{std::optional<Point>()} == gather_postorder_objects(std::optional<Point>()));
}

BOOST_AUTO_TEST_CASE(postorder_range) {
  const std::vector<Point> vec{Point{1, 2}, Point{3, 4}};
  const std::vector<SomeType> expected{1, 2, Point{1, 2}, 3, 4, Point{3, 4}, vec};
  BOOST_CHECK(expected == gather_postorder_objects(vec));
}

BOOST_AUTO_TEST_CASE(postorder_empty_range) {
  const std::vector<Point> vec;
  const std::vector<SomeType> expected{vec};
  BOOST_CHECK(expected == gather_postorder_objects(vec));
}

BOOST_AUTO_TEST_CASE(postorder_variant) {
  const std::variant<int, Point> var_point = Point{45, 89};
  const std::vector<SomeType> expected{45, 89, Point{45, 89}, var_point};
  BOOST_CHECK(expected == gather_postorder_objects(var_point));

  const std::variant<int, Point> var_int = 5;
  const std::vector<SomeType> expected2{5, var_int};
  BOOST_CHECK(expected2 == gather_postorder_objects(var_int));
}

BOOST_AUTO_TEST_CASE(postorder_non_tuple_tie) {
  BOOST_CHECK((std::vector<SomeType>{IntWrapper{5}}) == gather_postorder_objects(IntWrapper{5}));
  BOOST_CHECK((std::vector<SomeType>{5, VecWrapper{{5}}}) == gather_postorder_objects(VecWrapper{{5}}));
  BOOST_CHECK((std::vector<SomeType>{5, VariantWrapper{5}}) == gather_postorder_objects(VariantWrapper{5}));
}
