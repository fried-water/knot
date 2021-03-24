#include "gtest/gtest.h"

#include "knot.h"

using knot::details::as_tie;
using knot::details::is_tieable_v;

namespace {

struct Empty {};
static_assert(is_tieable_v<Empty>);

struct Derived : Empty {};
static_assert(!is_tieable_v<Derived>);

struct Virtual {
  virtual int foo() = 0;
};
static_assert(!is_tieable_v<Virtual>);

struct FewMembers {
  int x;
  float y;
};
static_assert(is_tieable_v<FewMembers>);

struct OptionalMember {
  std::optional<int> opt;
};
static_assert(is_tieable_v<OptionalMember>);

struct Compound {
  FewMembers f;
  Empty e;
  OptionalMember o;
};
static_assert(is_tieable_v<Compound>);

struct MemberFns {
  int x;

  float do_stuff();
  int get_stuff() const;
};
static_assert(is_tieable_v<MemberFns>);

struct ForwardTest {
  std::unique_ptr<int> ptr;
  float x;
};
static_assert(is_tieable_v<MemberFns>);

}  // namespace

TEST(AutoTie, forwarding) {
  ForwardTest s{std::make_unique<int>(5), 4};

  std::tuple<std::unique_ptr<int>&, float&> lvalue_tie = as_tie(s);
  std::tuple<std::unique_ptr<int>&&, float&&> rvalue_tie = as_tie(std::move(s));

  std::get<1>(lvalue_tie) = 1;
  std::unique_ptr<int> ptr = std::get<0>(std::move(rvalue_tie));

  EXPECT_EQ(1, s.x);
  EXPECT_EQ(nullptr, s.ptr);
  EXPECT_EQ(5, *ptr);
}
