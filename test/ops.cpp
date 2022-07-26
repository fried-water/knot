#include "knot/operators.h"

#include "test_structs.h"

#include "gtest/gtest.h"

TEST(Ops, ordered) {
  Pair<int, int> p1{1, 3};
  Pair<int, int> p2{2, 2};

  EXPECT_TRUE(p1 == p1);
  EXPECT_TRUE(p1 != p2);
  EXPECT_TRUE(p1 < p2);
  EXPECT_TRUE(p2 > p1);
  EXPECT_TRUE(p1 <= p1);
  EXPECT_TRUE(p2 >= p2);
}

TEST(Ops, rvalues) {
  EXPECT_TRUE((Pair<int, int>{1, 1}) == (Pair<int, int>{1, 1}));
  EXPECT_TRUE((Pair<int, int>{1, 1}) != (Pair<int, int>{1, 2}));
}
