#include "gtest/gtest.h"

#include "knot.h"

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
  const std::map<int, int> expected_map{{1, 3}, {2, 4}};

  const auto vec1 = knot::map<std::vector<P1>>(std::vector<P2>{{1, 3}, {2, 4}});
  EXPECT_EQ(expected_p1, vec1);

  const auto vec2 = knot::map<std::vector<P1>>(std::map<int, int>{{1, 3}, {2, 4}});
  EXPECT_EQ(expected_p1, vec2);

  const auto map1 = knot::map<std::map<int, int>>(expected_p1);
  EXPECT_EQ(expected_map, map1);
}
