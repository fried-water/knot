#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

namespace {

struct Memory {
  Memory(int count) : bytes(count) {}

  std::vector<char> bytes;

  friend auto as_tie(const Memory& m) { return std::tie(m.bytes); }
};

struct AreaOverride {
  std::size_t count;
  friend std::size_t area(const AreaOverride& a) { return a.count; }
};

struct Combo {
  Memory m;
  AreaOverride a;
};

struct TriviallyDestructibleUntieable {
  TriviallyDestructibleUntieable() {}  // no longer an aggregate
};
static_assert(!knot::is_tieable(knot::Type<TriviallyDestructibleUntieable>{}));
static_assert(std::is_trivially_destructible_v<TriviallyDestructibleUntieable>);

struct MemoryWrapper {
  friend auto as_tie(MemoryWrapper) { return Memory(5); }
};

}  // namespace

TEST(Area, primitive) {
  EXPECT_EQ(0, knot::area(5));
  EXPECT_EQ(0, knot::area(true));
  EXPECT_EQ(0, knot::area('c'));
}

TEST(Area, unique_ptr) {
  const std::unique_ptr<int> null;
  const auto int_ptr = std::make_unique<int>(5);
  const auto tuple_ptr = std::make_unique<std::tuple<double, int, char>>(5.0, 5, 'a');
  const auto ptr_ptr = std::make_unique<std::unique_ptr<int>>(std::make_unique<int>(5));

  EXPECT_EQ(0, knot::area(null));
  EXPECT_EQ(4, knot::area(int_ptr));
  EXPECT_EQ(16, knot::area(tuple_ptr));
  EXPECT_EQ(8 + 4, knot::area(ptr_ptr));
}

TEST(Area, vector) {
  const std::vector<int> empty;
  const std::vector<int> int_vec{1, 2, 3};
  const std::vector<Point> point_vec{{1, 2}, {3, 4}, {5, 6}};
  const std::vector<std::vector<int>> vec_vec{{1, 2}, {3, 4}, {5, 6}};

  EXPECT_EQ(0, knot::area(empty));
  EXPECT_EQ(12, knot::area(int_vec));
  EXPECT_EQ(24, knot::area(point_vec));
  EXPECT_EQ(72 + 24, knot::area(vec_vec));
}

TEST(Area, override) {
  using knot::area;
  EXPECT_EQ(17, area(AreaOverride{17}));
}

TEST(Area, product) {
  EXPECT_EQ(10, knot::area(std::make_pair(Memory(10), 5)));
  EXPECT_EQ(17, knot::area(std::make_tuple(Memory(10), AreaOverride{7}, 'a')));
  EXPECT_EQ(21, knot::area(Combo{Memory(10), AreaOverride{11}}));
}

TEST(Area, sum) {
  EXPECT_EQ(0, knot::area(std::optional<Memory>()));
  EXPECT_EQ(5, knot::area(std::optional<Memory>(Memory(5))));
  EXPECT_EQ(0, knot::area(std::variant<int, Memory>(5)));
  EXPECT_EQ(5, knot::area(std::variant<int, Memory>(Memory(5))));
}

TEST(Area, array) {
  EXPECT_EQ(17, knot::area(std::array<Memory, 3>{Memory(10), Memory(5), Memory(2)}));
}

TEST(Area, trivially_destructible) {
  EXPECT_EQ(0, knot::area(TriviallyDestructibleUntieable{}));
}

TEST(Area, non_tuple_tie) {
  EXPECT_EQ(5, knot::area(MemoryWrapper{}));
  EXPECT_EQ(0, knot::area(IntWrapper{}));
  EXPECT_EQ(12, knot::area(VecWrapper{{1, 2, 3}}));
  EXPECT_EQ(0, knot::area(VariantWrapper{1}));
}
