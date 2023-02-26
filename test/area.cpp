#include "knot/area.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_CASE(area_primitive) {
  BOOST_CHECK(0 == knot::area(5));
  BOOST_CHECK(0 == knot::area(true));
  BOOST_CHECK(0 == knot::area('c'));
}

BOOST_AUTO_TEST_CASE(area_unique_ptr) {
  const std::unique_ptr<int> null;
  const auto int_ptr = std::make_unique<int>(5);
  const auto tuple_ptr = std::make_unique<std::tuple<double, int, char>>(5.0, 5, 'a');
  const auto ptr_ptr = std::make_unique<std::unique_ptr<int>>(std::make_unique<int>(5));

  BOOST_CHECK(0 == knot::area(null));
  BOOST_CHECK(4 == knot::area(int_ptr));
  BOOST_CHECK(16 == knot::area(tuple_ptr));
  BOOST_CHECK(8 + 4 == knot::area(ptr_ptr));
}

BOOST_AUTO_TEST_CASE(area_vector) {
  const std::vector<int> empty;
  const std::vector<int> int_vec{1, 2, 3};
  const std::vector<Point> point_vec{{1, 2}, {3, 4}, {5, 6}};
  const std::vector<std::vector<int>> vec_vec{{1, 2}, {3, 4}, {5, 6}};

  BOOST_CHECK(0 == knot::area(empty));
  BOOST_CHECK(12 == knot::area(int_vec));
  BOOST_CHECK(24 == knot::area(point_vec));
  BOOST_CHECK(72 + 24 == knot::area(vec_vec));
}

BOOST_AUTO_TEST_CASE(area_override) {
  using knot::area;
  BOOST_CHECK(17 == area(AreaOverride{17}));
}

BOOST_AUTO_TEST_CASE(area_product) {
  BOOST_CHECK(10 == knot::area(std::make_pair(Memory(10), 5)));
  BOOST_CHECK(17 == knot::area(std::make_tuple(Memory(10), AreaOverride{7}, 'a')));
  BOOST_CHECK(21 == knot::area(Combo{Memory(10), AreaOverride{11}}));
}

BOOST_AUTO_TEST_CASE(area_sum) {
  BOOST_CHECK(0 == knot::area(std::optional<Memory>()));
  BOOST_CHECK(5 == knot::area(std::optional<Memory>(Memory(5))));
  BOOST_CHECK(0 == knot::area(std::variant<int, Memory>(5)));
  BOOST_CHECK(5 == knot::area(std::variant<int, Memory>(Memory(5))));
}

BOOST_AUTO_TEST_CASE(area_array) {
  BOOST_CHECK(17 == knot::area(std::array<Memory, 3>{Memory(10), Memory(5), Memory(2)}));
}

BOOST_AUTO_TEST_CASE(area_trivially_destructible) { BOOST_CHECK(0 == knot::area(TriviallyDestructibleUntieable{})); }

BOOST_AUTO_TEST_CASE(area_non_tuple_tie) {
  BOOST_CHECK(5 == knot::area(MemoryWrapper{}));
  BOOST_CHECK(0 == knot::area(IntWrapper{}));
  BOOST_CHECK(12 == knot::area(VecWrapper{{1, 2, 3}}));
  BOOST_CHECK(0 == knot::area(VariantWrapper{1}));
}
