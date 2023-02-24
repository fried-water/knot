#include "knot/map.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <vector>

struct P1 {
  KNOT_COMPAREABLE(P1);
  int x;
  int y;
};

struct P2 {
  KNOT_COMPAREABLE(P2);
  int x;
  int y;
};

BOOST_AUTO_TEST_CASE(map_primitive) {
  const int x = knot::map<int>(1.0f);
  BOOST_CHECK(1 == x);
}

BOOST_AUTO_TEST_CASE(map_product) {
  const auto p2 = knot::map<P2>(P1{1, 3});
  const P2 expected_p2{1, 3};
  BOOST_CHECK(expected_p2 == p2);

  const auto pair = knot::map<std::pair<int, float>>(P1{1, 3});
  BOOST_CHECK(std::pair(1, 3.0f) == pair);

  const auto p1 = knot::map<P1>(std::make_pair(1, 3.0f));
  const P1 expected_p1{1, 3};
  BOOST_CHECK(expected_p1 == p1);
}

BOOST_AUTO_TEST_CASE(map_const_value) {
  const int i = knot::map<const int>(3.0f);
  BOOST_CHECK(3 == i);

  const auto pair = knot::map<std::pair<const int, const float>>(P1{1, 3});
  std::pair<const int, const float> expected_pair{1, 3.0f};
  BOOST_CHECK(expected_pair == pair);
}

BOOST_AUTO_TEST_CASE(map_range) {
  const std::vector<P1> expected_p1{{1, 3}, {2, 4}};

  const auto vec1 = knot::map<std::vector<P1>>(std::vector<P2>{{1, 3}, {2, 4}});
  BOOST_CHECK(expected_p1 == vec1);

  const auto vec2 = knot::map<std::vector<P1>>(std::map<int, int>{{1, 3}, {2, 4}});
  BOOST_CHECK(expected_p1 == vec2);

  const auto map1 = knot::map<std::map<int, int>>(expected_p1);
  const std::map<int, int> expected_map{{1, 3}, {2, 4}};
  BOOST_CHECK(expected_map == map1);

  // inner accumuate override
  const auto vec3 =
      knot::map<std::vector<int>>(std::vector<std::vector<int>>{{1, 3}, {2, 4}},
                                  [](const std::vector<int>& v) { return std::accumulate(v.begin(), v.end(), 0); });
  const std::vector<int> expected_acc{4, 6};
  BOOST_CHECK(expected_acc == vec3);
}

BOOST_AUTO_TEST_CASE(map_maybe) {
  const std::optional<P1> expected_opt{P1{1, 3}};
  const std::unique_ptr<P2> expected_ptr{new P2{1, 3}};

  const auto ptr = knot::map<std::unique_ptr<P2>>(expected_opt);
  BOOST_CHECK(*expected_ptr == *ptr);

  const auto opt = knot::map<std::optional<P1>>(expected_ptr);
  BOOST_CHECK(expected_opt == opt);

  const auto ptr2 = knot::map<std::unique_ptr<P2>>(std::optional<P1>{});
  BOOST_CHECK(nullptr == ptr2);

  const auto opt2 = knot::map<std::optional<P1>>(std::unique_ptr<P2>{});
  BOOST_CHECK(std::nullopt == opt2);
}

BOOST_AUTO_TEST_CASE(map_variant) {
  const auto var = knot::map<std::variant<float, int, char>>(std::variant<int>{1});
  const auto expected = std::variant<float, int, char>{1};
  BOOST_CHECK(expected == var);

  const auto var_override = knot::map<std::variant<int, std::size_t>>(
      std::variant<std::string>{std::string{"abc"}}, [](const std::string& str) { return str.size(); });
  const auto expected_override = std::variant<int, std::size_t>{3ul};
  BOOST_CHECK(expected_override == var_override);

  const auto var_map = knot::map<std::variant<P1, int>>(std::variant<std::pair<int, int>, int>{std::pair(1, 1)});
  const auto expected_map = std::variant<P1, int>{P1{1, 1}};
  BOOST_CHECK(expected_map == var_map);
}

BOOST_AUTO_TEST_CASE(map_override) {
  const auto p2 = knot::map<P2>(std::make_pair(1, std::string{"abc"}),
                                [](const std::string& str) { return static_cast<int>(str.size()); });
  const P2 expected_p2{1, 3};

  BOOST_CHECK(expected_p2 == p2);
}

struct MoveOnly {
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
};

struct OuterMoveOnly {
  MoveOnly x;
};

BOOST_AUTO_TEST_CASE(map_move_only) {
  // identity
  knot::map<MoveOnly>(MoveOnly{});

  // override
  knot::map<MoveOnly>(MoveOnly{}, [](MoveOnly m) { return m; });

  // product
  knot::map<std::tuple<MoveOnly, MoveOnly>>(std::make_pair(MoveOnly{}, MoveOnly{}));
  knot::map<std::pair<MoveOnly, MoveOnly>>(std::make_tuple(MoveOnly{}, MoveOnly{}));
  knot::map<std::pair<MoveOnly, MoveOnly>>(std::make_tuple(MoveOnly{}, MoveOnly{}));
  knot::map<std::tuple<MoveOnly>>(OuterMoveOnly{});
  knot::map<OuterMoveOnly>(std::make_tuple(MoveOnly{}));

  // range
  knot::map<std::map<int, MoveOnly>>(std::vector<std::pair<int, MoveOnly>>{});

  // maybe
  knot::map<std::unique_ptr<std::pair<MoveOnly, MoveOnly>>>(std::optional<std::pair<MoveOnly, MoveOnly>>{});

  // variant
  knot::map<std::variant<int, MoveOnly>>(std::variant<MoveOnly>{MoveOnly{}});

  // variant override
  knot::map<std::variant<int, MoveOnly>>(std::variant<MoveOnly>{MoveOnly{}}, [](MoveOnly m) { return m; });
}

struct ExplicitContructor {
  int x;
  explicit ExplicitContructor(int x) : x(x) {}
  friend auto as_tie(const ExplicitContructor& e) { return std::tie(e.x); }
};

BOOST_AUTO_TEST_CASE(map_explicit_constructor) { BOOST_CHECK(1 == knot::map<ExplicitContructor>(std::tuple(1)).x); }

BOOST_AUTO_TEST_CASE(map_non_tuple_tie) {
  BOOST_CHECK(IntWrapper{5} == knot::map<IntWrapper>(5));
  BOOST_CHECK(5 == knot::map<int>(IntWrapper{5}));

  BOOST_CHECK(VariantWrapper{0.5f} == knot::map<VariantWrapper>(std::variant<int, float>{0.5f}));
  BOOST_CHECK((std::variant<int, float>{5}) == (knot::map<std::variant<int, float>>(VariantWrapper{5})));

  BOOST_CHECK((VecWrapper{{1, 2, 3}}) == knot::map<VecWrapper>(std::vector<int>{1, 2, 3}));
  BOOST_CHECK((std::vector<int>{1, 2, 3}) == (knot::map<std::vector<int>>(VecWrapper{{1, 2, 3}})));
}
