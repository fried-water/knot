#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

using knot::details::as_tie;
using knot::details::is_tieable_v;

namespace {

struct Empty {};
static_assert(is_tieable_v<Empty>);

struct Derived : Empty {};
static_assert(!is_tieable_v<Derived>);

struct Virtual { virtual int foo() = 0; };
static_assert(!is_tieable_v<Virtual>);

struct FewMembers { int x; float y; };
static_assert(is_tieable_v<FewMembers>);

struct OptionalMember { std::optional<int> opt; };
static_assert(is_tieable_v<OptionalMember>);

struct Compound { FewMembers f; Empty e; OptionalMember o; };
static_assert(is_tieable_v<Compound>);

struct MemberFns {
  int x;

  float do_stuff();
  int get_stuff() const;
};
static_assert(is_tieable_v<MemberFns>);

}
