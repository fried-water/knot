#include "gtest/gtest.h"

#include "knot.h"

#include <memory>
#include <optional>
#include <vector>
#include <map>

struct P1 { 
  KNOT_COMPAREABLE(P1);
  int x; int y;
};

struct P2 { 
  KNOT_COMPAREABLE(P2);
  int x; int y;
};

TEST(Map, primitive) {
  const int x = knot::map<int>(1.0f);
  EXPECT_EQ(1, x);
}

TEST(Map, product) {
  const auto p2 = knot::map<P2>(P1{1, 3});
  const P2 expected_p2{1, 3};
  EXPECT_EQ(expected_p2, p2);

  const auto pair = knot::map<std::pair<int, float>>(P1{1, 3});
  EXPECT_EQ(std::make_pair(1, 3.0f), pair);

  const auto p1 = knot::map<P1>(std::make_pair(1, 3.0f));
  const P1 expected_p1{1, 3};
  EXPECT_EQ(expected_p1, p1);
}

TEST(Map, range) {
  const std::vector<P1> expected_p1{{1, 3}, {2, 4}};
  

  const auto vec1 = knot::map<std::vector<P1>>(std::vector<P2>{{1, 3}, {2, 4}});
  EXPECT_EQ(expected_p1, vec1);

  const auto vec2 = knot::map<std::vector<P1>>(std::map<int, int>{{1, 3}, {2, 4}});
  EXPECT_EQ(expected_p1, vec2);

  const auto map1 = knot::map<std::map<int, int>>(expected_p1);
  const std::map<int, int> expected_map{{1, 3}, {2, 4}};
  EXPECT_EQ(expected_map, map1);

  // inner accumuate override
  const auto vec3 = knot::map<std::vector<int>>(std::vector<std::vector<int>>{{1, 3}, {2, 4}},
    [](const std::vector<int>& v) { return std::accumulate(v.begin(), v.end(), 0); });
  const std::vector<int> expected_acc{4, 6};
  EXPECT_EQ(expected_acc, vec3);
}

TEST(Map, maybe) {
  const std::optional<P1> expected_opt{P1{1, 3}};
  const std::unique_ptr<P2> expected_ptr{new P2{1, 3}};

  const auto ptr = knot::map<std::unique_ptr<P2>>(expected_opt);
  EXPECT_EQ(*expected_ptr, *ptr);

  const auto opt = knot::map<std::optional<P1>>(expected_ptr);
  EXPECT_EQ(expected_opt, opt);

  const auto ptr2 = knot::map<std::unique_ptr<P2>>(std::optional<P1>{});
  EXPECT_EQ(nullptr, ptr2);

  const auto opt2 = knot::map<std::optional<P1>>(std::unique_ptr<P2>{});
  EXPECT_EQ(std::nullopt, opt2);
}

TEST(Map, variant) {
  const auto var = knot::map<std::variant<int, char>>(std::variant<int>{1});
  const auto expected = std::variant<int, char>{1}; 
  EXPECT_EQ(expected, var);

  const auto var_override = knot::map<std::variant<int, std::size_t>>(std::variant<std::string>{std::string{"abc"}}, [](const std::string& str) { return str.size(); });
  const auto expected_override = std::variant<int, std::size_t>{3ul}; 
  EXPECT_EQ(expected_override, var_override);
}

TEST(Map, override) {
  const auto p2 = knot::map<P2>(std::make_pair(1, std::string{"abc"}), 
    [](const std::string& str) {
      return str.size();
    });
  const P2 expected_p2{1, 3};

  EXPECT_EQ(expected_p2, p2);
}

struct MoveOnly {
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) = default;
};

struct OuterMoveOnly {
  MoveOnly x;

  // to map move only object in structs an && as_tie is required
  friend auto as_tie(const OuterMoveOnly& m) { return std::tie(m.x); }
  friend auto as_tie(OuterMoveOnly&& m) { return std::forward_as_tuple(std::move(m.x)); }
};

TEST(Map, move_only) {
  // identity
  knot::map<MoveOnly>(MoveOnly{});

  // override
  knot::map<MoveOnly>(MoveOnly{}, [](MoveOnly m) { return m; });

  // product
  knot::map<std::tuple<MoveOnly, MoveOnly>>(std::pair(MoveOnly{}, MoveOnly{}));
  knot::map<std::pair<MoveOnly, MoveOnly>>(std::tuple(MoveOnly{}, MoveOnly{}));
  knot::map<std::pair<MoveOnly, MoveOnly>>(std::tuple(MoveOnly{}, MoveOnly{}));
  knot::map<std::tuple<MoveOnly>>(OuterMoveOnly{});
  knot::map<OuterMoveOnly>(std::tuple(MoveOnly{}));

  // range
  knot::map<std::map<int, MoveOnly>>(std::vector<std::pair<int, MoveOnly>>{});

  // maybe
  knot::map<std::unique_ptr<std::pair<MoveOnly, MoveOnly>>>(std::optional<std::pair<MoveOnly, MoveOnly>>{});

  // variant
  knot::map<std::variant<int, MoveOnly>>(std::variant<MoveOnly>{MoveOnly{}});

  // variant override
  knot::map<std::variant<int, MoveOnly>>(std::variant<MoveOnly>{MoveOnly{}}, [](MoveOnly m) { return m; });
}
