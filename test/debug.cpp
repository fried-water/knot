#include "knot/debug.h"

#include "test_structs.h"

#include "gtest/gtest.h"

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
  EXPECT_EQ("()", knot::debug(std::tuple()));
  EXPECT_EQ("(abc)", knot::debug(std::tuple("abc")));
  EXPECT_EQ("(1, 2, 3, 4)", knot::debug(std::tuple(1, 2, 3, 4)));
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
  EXPECT_EQ("[0;]", knot::debug(std::vector<int>{}));
  EXPECT_EQ("[1; 1]", knot::debug(std::vector<int>{1}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(std::vector<int>{1, 2, 3}));

  EXPECT_EQ("[0;]", knot::debug(std::array<int, 0>{}));
  EXPECT_EQ("[1; 1]", knot::debug(std::array<int, 1>{1}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(std::array<int, 3>{1, 2, 3}));

  EXPECT_EQ("[0;]", knot::debug(std::set<int>{}));
  EXPECT_EQ("[1; 1]", knot::debug(std::set<int>{1}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(std::set<int>{3, 2, 1}));

  EXPECT_EQ("[0;]", knot::debug(std::map<int, std::string>{}));
  EXPECT_EQ("[1; (1, a)]", knot::debug(std::map<int, std::string>{{1, "a"}}));
  EXPECT_EQ("[3; (1, a), (2, b), (3, c)]", knot::debug(std::map<int, std::string>{{3, "c"}, {2, "b"}, {1, "a"}}));

  // Skipping unordered containers since order won't be consistent across platforms
}

TEST(Debug, variant) {
  EXPECT_EQ("5", knot::debug(std::variant<int, Point, std::vector<std::string>>(5)));
  EXPECT_EQ("(45, 89)", knot::debug(std::variant<int, Point, std::vector<std::string>>(Point{45, 89})));
  EXPECT_EQ("[3; a, b, c]",
            knot::debug(std::variant<int, Point, std::vector<std::string>>(std::vector<std::string>{"a", "b", "c"})));
}

TEST(Debug, non_tuple_tieable) {
  EXPECT_EQ("5", knot::debug(IntWrapper{5}));
  EXPECT_EQ("[3; 1, 2, 3]", knot::debug(VecWrapper{{1, 2, 3}}));
  EXPECT_EQ("5", knot::debug(VariantWrapper{5}));
}

namespace {

struct MyNamedStruct {
  int member1;
  int member2;

  friend constexpr auto names(knot::Type<MyNamedStruct>) { return knot::Names("MyNamedStruct", {"member1", "member2"}); }
};

struct MySemiNamedStruct {
  int member1;
  int member2;

  friend constexpr auto names(knot::Type<MySemiNamedStruct>) { return knot::Names("MySemiNamedStruct"); }
};

struct MyNamedAliasedStruct {
  int value;

  friend auto as_tie(const MyNamedAliasedStruct& m) { return m.value; }

  friend constexpr auto names(knot::Type<MyNamedAliasedStruct>) { return knot::Names("MyNamedAliasedStruct"); }
};

}  // namespace

TEST(Debug, named_struct) { EXPECT_EQ("MyNamedStruct(member1: 5, member2: 3)", knot::debug(MyNamedStruct{5, 3})); }

TEST(Debug, semi_named_struct) { EXPECT_EQ("MySemiNamedStruct(5, 3)", knot::debug(MySemiNamedStruct{5, 3})); }

TEST(Debug, named_aliased_struct) { EXPECT_EQ("MyNamedAliasedStruct(5)", knot::debug(MyNamedAliasedStruct{5})); }

TEST(Debug, multiline_range) {
  EXPECT_EQ("[0;]", knot::debug(std::vector<int>{}, knot::MultiLine{}));
  EXPECT_EQ("[0;\n]", knot::debug(std::vector<int>{}, knot::MultiLine{0}));
  EXPECT_EQ("[1;\n  2\n]", knot::debug(std::vector<int>{2}, knot::MultiLine{1}));
  EXPECT_EQ("[1; 2]", knot::debug(std::vector<int>{2}, knot::MultiLine{2}));
  EXPECT_EQ("[4;\n [0;],\n [1; 1],\n [2; 1, 2],\n [3;\n  1,\n  2,\n  3\n ]\n]", 
    knot::debug(std::vector<std::vector<int>>{{}, {1}, {1, 2}, {1, 2, 3}}, knot::MultiLine{3, 1}));
}

TEST(Debug, multiline_struct) {
  EXPECT_EQ("()", knot::debug(std::tuple(), knot::MultiLine{}));
  EXPECT_EQ("(\n)", knot::debug(std::tuple(), knot::MultiLine{0}));

  EXPECT_EQ("(\n  a,\n  1\n)", knot::debug(std::tuple('a', 1), knot::MultiLine{2}));
  EXPECT_EQ("(a, 1)", knot::debug(std::tuple('a', 1), knot::MultiLine{3}));

  EXPECT_EQ("MyNamedStruct(\n  member1: 0,\n  member2: 1\n)", knot::debug(MyNamedStruct{0, 1}, knot::MultiLine{2}));
  EXPECT_EQ("MyNamedStruct(member1: 0, member2: 1)", knot::debug(MyNamedStruct{0, 1}, knot::MultiLine{3}));

  EXPECT_EQ("(\n abc,\n (),\n (\n  1,\n  2\n )\n)", knot::debug(std::tuple("abc", std::tuple(), std::tuple(1, 2)), knot::MultiLine{2, 1}));
}
