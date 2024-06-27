#include "knot/debug.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

#include <map>
#include <set>

BOOST_AUTO_TEST_CASE(debug_primitive) {
  BOOST_CHECK_EQUAL("5", knot::debug(5));
  BOOST_CHECK_EQUAL("true", knot::debug(true));
  BOOST_CHECK_EQUAL("false", knot::debug(false));
  BOOST_CHECK_EQUAL("c", knot::debug('c'));
  BOOST_CHECK_EQUAL("1.5", knot::debug(1.5));
}

BOOST_AUTO_TEST_CASE(debug_string) {
  BOOST_CHECK_EQUAL("abc", knot::debug("abc"));
  BOOST_CHECK_EQUAL("abc", knot::debug(std::string("abc")));
  BOOST_CHECK_EQUAL("abc", knot::debug(std::string_view("abc")));
}

BOOST_AUTO_TEST_CASE(debug_product_types) {
  BOOST_CHECK_EQUAL("()", knot::debug(std::tuple()));
  BOOST_CHECK_EQUAL("(abc)", knot::debug(std::tuple("abc")));
  BOOST_CHECK_EQUAL("(1, 2, 3, 4)", knot::debug(std::tuple(1, 2, 3, 4)));
  BOOST_CHECK_EQUAL("(x, 9)", knot::debug(std::make_pair('x', 9)));
  BOOST_CHECK_EQUAL("(45, 89)", knot::debug(Point{45, 89}));
  BOOST_CHECK_EQUAL("((1, 2), (3, 4))", knot::debug(Bbox{Point{1, 2}, Point{3, 4}}));
}

BOOST_AUTO_TEST_CASE(debug_optional_types) {
  BOOST_CHECK_EQUAL("(45, 89)", knot::debug(std::optional<Point>(Point{45, 89})));
  BOOST_CHECK_EQUAL("none", knot::debug(std::optional<Point>()));

  BOOST_CHECK_EQUAL("(45, 89)", knot::debug(std::make_unique<Point>(Point{45, 89})));
  BOOST_CHECK_EQUAL("none", knot::debug(std::unique_ptr<Point>()));
}

BOOST_AUTO_TEST_CASE(debug_range_types) {
  BOOST_CHECK_EQUAL("[0;]", knot::debug(std::vector<int>{}));
  BOOST_CHECK_EQUAL("[1; 1]", knot::debug(std::vector<int>{1}));
  BOOST_CHECK_EQUAL("[3; 1, 2, 3]", knot::debug(std::vector<int>{1, 2, 3}));

  BOOST_CHECK_EQUAL("[0;]", knot::debug(std::array<int, 0>{}));
  BOOST_CHECK_EQUAL("[1; 1]", knot::debug(std::array<int, 1>{1}));
  BOOST_CHECK_EQUAL("[3; 1, 2, 3]", knot::debug(std::array<int, 3>{1, 2, 3}));

  BOOST_CHECK_EQUAL("[0;]", knot::debug(std::set<int>{}));
  BOOST_CHECK_EQUAL("[1; 1]", knot::debug(std::set<int>{1}));
  BOOST_CHECK_EQUAL("[3; 1, 2, 3]", knot::debug(std::set<int>{3, 2, 1}));

  BOOST_CHECK_EQUAL("[0;]", knot::debug(std::map<int, std::string>{}));
  BOOST_CHECK_EQUAL("[1; (1, a)]", knot::debug(std::map<int, std::string>{{1, "a"}}));
  BOOST_CHECK_EQUAL("[3; (1, a), (2, b), (3, c)]",
                    knot::debug(std::map<int, std::string>{{3, "c"}, {2, "b"}, {1, "a"}}));

  // Skipping unordered containers since order won't be consistent across platforms
}

BOOST_AUTO_TEST_CASE(debug_vec_bool) {
  BOOST_CHECK_EQUAL("[2; false, true]", knot::debug(std::vector<bool>{false, true}));
}

BOOST_AUTO_TEST_CASE(debug_variant) {
  BOOST_CHECK_EQUAL("5", knot::debug(std::variant<int, Point, std::vector<std::string>>(5)));
  BOOST_CHECK_EQUAL("(45, 89)", knot::debug(std::variant<int, Point, std::vector<std::string>>(Point{45, 89})));
  BOOST_CHECK_EQUAL(
      "[3; a, b, c]",
      knot::debug(std::variant<int, Point, std::vector<std::string>>(std::vector<std::string>{"a", "b", "c"})));
}

BOOST_AUTO_TEST_CASE(debug_non_tuple_tieable) {
  BOOST_CHECK_EQUAL("5", knot::debug(IntWrapper{5}));
  BOOST_CHECK_EQUAL("[3; 1, 2, 3]", knot::debug(VecWrapper{{1, 2, 3}}));
  BOOST_CHECK_EQUAL("5", knot::debug(VariantWrapper{5}));
}

namespace {

struct MyNamedStruct {
  int member1;
  int member2;

  friend constexpr auto names(knot::Type<MyNamedStruct>) {
    return knot::Names("MyNamedStruct", {"member1", "member2"});
  }
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

enum class UnnamedEnum { One, Two, Three };
enum class TitledEnum { One, Two, Three };
enum class PartialNamedEnum { One, Two, Three };
enum class NamedEnum { One, Two, Three };

constexpr auto names(knot::Type<TitledEnum>) { return knot::Names("TitledEnum"); }
constexpr auto names(knot::Type<PartialNamedEnum>) { return knot::Names({"One", "Two", "Three"}); }
constexpr auto names(knot::Type<NamedEnum>) { return knot::Names("NamedEnum", {"One", "Two", "Three"}); }

}  // namespace

BOOST_AUTO_TEST_CASE(debug_named_struct) {
  BOOST_CHECK_EQUAL("MyNamedStruct(member1: 5, member2: 3)", knot::debug(MyNamedStruct{5, 3}));
}

BOOST_AUTO_TEST_CASE(debug_semi_named_struct) {
  BOOST_CHECK_EQUAL("MySemiNamedStruct(5, 3)", knot::debug(MySemiNamedStruct{5, 3}));
}

BOOST_AUTO_TEST_CASE(debug_named_aliased_struct) {
  BOOST_CHECK_EQUAL("MyNamedAliasedStruct(5)", knot::debug(MyNamedAliasedStruct{5}));
}

BOOST_AUTO_TEST_CASE(debug_multiline_range) {
  BOOST_CHECK_EQUAL("[0;]", knot::debug(std::vector<int>{}, knot::MultiLine{}));
  BOOST_CHECK_EQUAL("[0;\n]", knot::debug(std::vector<int>{}, knot::MultiLine{0}));
  BOOST_CHECK_EQUAL("[1;\n  2\n]", knot::debug(std::vector<int>{2}, knot::MultiLine{1}));
  BOOST_CHECK_EQUAL("[1; 2]", knot::debug(std::vector<int>{2}, knot::MultiLine{2}));
  BOOST_CHECK_EQUAL("[4;\n [0;],\n [1; 1],\n [2; 1, 2],\n [3;\n  1,\n  2,\n  3\n ]\n]",
                    knot::debug(std::vector<std::vector<int>>{{}, {1}, {1, 2}, {1, 2, 3}}, knot::MultiLine{3, 1}));
}

BOOST_AUTO_TEST_CASE(debug_multiline_struct) {
  BOOST_CHECK_EQUAL("()", knot::debug(std::tuple(), knot::MultiLine{}));
  BOOST_CHECK_EQUAL("(\n)", knot::debug(std::tuple(), knot::MultiLine{0}));

  BOOST_CHECK_EQUAL("(\n  a,\n  1\n)", knot::debug(std::tuple('a', 1), knot::MultiLine{2}));
  BOOST_CHECK_EQUAL("(a, 1)", knot::debug(std::tuple('a', 1), knot::MultiLine{3}));

  BOOST_CHECK_EQUAL("MyNamedStruct(\n  member1: 0,\n  member2: 1\n)",
                    knot::debug(MyNamedStruct{0, 1}, knot::MultiLine{29}));
  BOOST_CHECK_EQUAL("MyNamedStruct(member1: 0, member2: 1)", knot::debug(MyNamedStruct{0, 1}, knot::MultiLine{30}));

  BOOST_CHECK_EQUAL("(\n abc,\n (),\n (\n  1,\n  2\n )\n)",
                    knot::debug(std::tuple("abc", std::tuple(), std::tuple(1, 2)), knot::MultiLine{2, 1}));
}

BOOST_AUTO_TEST_CASE(debug_named_enum) {
  BOOST_CHECK_EQUAL("NamedEnum::One", knot::debug(NamedEnum::One));
  BOOST_CHECK_EQUAL("NamedEnum(-1)", knot::debug(static_cast<NamedEnum>(-1)));

  BOOST_CHECK_EQUAL("TitledEnum(0)", knot::debug(TitledEnum::One));
  BOOST_CHECK_EQUAL("TitledEnum(-1)", knot::debug(static_cast<TitledEnum>(-1)));

  BOOST_CHECK_EQUAL("One", knot::debug(PartialNamedEnum::One));
  BOOST_CHECK_EQUAL("invalid_enum(-1)", knot::debug(static_cast<PartialNamedEnum>(-1)));

  BOOST_CHECK_EQUAL("0", knot::debug(UnnamedEnum::One));
  BOOST_CHECK_EQUAL("-1", knot::debug(static_cast<UnnamedEnum>(-1)));
}
