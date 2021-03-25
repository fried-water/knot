#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

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
  EXPECT_TRUE((Pair{1, 1}) == (Pair{1, 1}));
  EXPECT_TRUE((Pair{1, 1}) != (Pair{1, 2}));
}
