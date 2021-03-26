#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

#include <map>
#include <set>

TEST(Debug, primitive) {
  EXPECT_EQ("5", knot::debug(5));
  EXPECT_EQ("true", knot::debug(true));
  EXPECT_EQ("false", knot::debug(false));
  EXPECT_EQ("c", knot::debug('c'));
  EXPECT_EQ("1.5", knot::debug(1.5));
}

TEST(Debug, string) {
  EXPECT_EQ("abc", knot::debug("abc"));
  EXPECT_EQ("abc", knot::debug(std::string("abc")));
  EXPECT_EQ("abc", knot::debug(std::string_view("abc")));
}

TEST(Debug, product_types) {
  EXPECT_EQ("()", knot::debug(std::make_tuple()));
  EXPECT_EQ("(abc)", knot::debug(std::make_tuple("abc")));
  EXPECT_EQ("(1, 2, 3, 4)", knot::debug(std::make_tuple(1, 2, 3, 4)));
  EXPECT_EQ("(x, 9)", knot::debug(std::make_pair('x', 9)));
  EXPECT_EQ("(45, 89)", knot::debug(Point{45, 89}));
  EXPECT_EQ("((1, 2), (3, 4))", knot::debug(Bbox{Point{1, 2}, Point{3, 4}}));
}

TEST(Debug, optional_types) {
  EXPECT_EQ("(45, 89)", knot::debug(std::optional<Point>(Point{45, 89})));
  EXPECT_EQ("none", knot::debug(std::optional<Point>()));

  EXPECT_EQ("(45, 89)", knot::debug(std::make_unique<Point>(Point{45, 89})));
  EXPECT_EQ("none", knot::debug(std::unique_ptr<Point>()));
}

TEST(Debug, range_types) {
  EXPECT_EQ("[0; ]", knot::debug(std::vector<int>{}));
  EXPECT_EQ("[1; 1]", knot::debug(std::vector<int>{1}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(std::vector<int>{1, 2, 3}));

  EXPECT_EQ("[0; ]", knot::debug(std::array<int, 0>{}));
  EXPECT_EQ("[1; 1]", knot::debug(std::array<int, 1>{1}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(std::array<int, 3>{1, 2, 3}));

  EXPECT_EQ("[0; ]", knot::debug(std::set<int>{}));
  EXPECT_EQ("[1; 1]", knot::debug(std::set<int>{1}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(std::set<int>{3, 2, 1}));

  EXPECT_EQ("[0; ]", knot::debug(std::map<int, std::string>{}));
  EXPECT_EQ("[1; (1, a)]", knot::debug(std::map<int, std::string>{{1, "a"}}));
  EXPECT_EQ("[3; (1, a), (2, b), (3, c)]", knot::debug(std::map<int, std::string>{{3, "c"} , {2 , "b"}, {1, "a"}}));

  // Skipping unordered containers since order won't be consistent across platforms
}

TEST(Debug, variant) {
  EXPECT_EQ("<5>", knot::debug(std::variant<int, Point, std::vector<std::string>>(5)));
  EXPECT_EQ("<(45, 89)>", knot::debug(std::variant<int, Point, std::vector<std::string>>(Point{45, 89})));
  EXPECT_EQ("<[3; a, b, c]>", knot::debug(std::variant<int, Point, std::vector<std::string>>(std::vector<std::string>{"a", "b", "c"})));
}
