#include "knot/serialize.h"

#include "test_structs.h"

#include <boost/test/unit_test.hpp>

#include <map>

namespace {

std::vector<std::byte> as_bytes(std::initializer_list<uint8_t> chars) {
  std::vector<std::byte> bytes;
  std::transform(chars.begin(), chars.end(), std::back_inserter(bytes), [](uint8_t c) { return std::byte{c}; });

  return bytes;
}

}  // namespace

BOOST_AUTO_TEST_CASE(serialize_primitive) {
  const std::vector<std::byte> bytes = knot::serialize(5);
  // Does this depend on endianess of machine?
  const auto expected = as_bytes({5, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(5 == knot::deserialize<int>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_not_enough_bytes) {
  const std::vector<uint8_t> empty_bytes;
  BOOST_CHECK(std::nullopt == knot::deserialize<int>(empty_bytes.begin(), empty_bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_extra_bytes) {
  const std::vector<uint8_t> extra_bytes{5, 0, 0, 0, 0};
  BOOST_CHECK(std::nullopt == knot::deserialize<int>(extra_bytes.begin(), extra_bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_basic_struct) {
  const Point p{45, 89};
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({45, 0, 0, 0, 89, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(p == knot::deserialize<Point>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<std::byte> bytes = knot::serialize(bbox);
  BOOST_CHECK(16 == bytes.size());

  BOOST_CHECK(bbox == knot::deserialize<Bbox>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({1, 45, 0, 0, 0, 89, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(p == knot::deserialize<std::optional<Point>>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_nullopt) {
  const std::optional<Point> p;
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(p == *knot::deserialize<std::optional<Point>>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_unique_ptr) {
  const auto p = std::make_unique<Point>(Point{45, 89});
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({1, 45, 0, 0, 0, 89, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(*p == **knot::deserialize<std::unique_ptr<Point>>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_null_unique_ptr) {
  const std::unique_ptr<Point> p;
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(p == knot::deserialize<std::unique_ptr<Point>>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_move_only) {
  using type = std::vector<std::tuple<std::optional<std::unique_ptr<int>>>>;

  type vec;
  vec.push_back(std::tuple(std::optional(std::make_unique<int>(7))));
  const std::vector<std::byte> bytes = knot::serialize(vec);
  const auto expected = as_bytes({1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 7, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(7 == **std::get<0>((*knot::deserialize<type>(bytes.begin(), bytes.end()))[0]));
}

BOOST_AUTO_TEST_CASE(serialize_range) {
  const std::vector<int> vec{1, 2, 3};
  const std::vector<std::byte> bytes = knot::serialize(vec);
  const auto expected = as_bytes({3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(vec == knot::deserialize<std::vector<int>>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_empty_range) {
  const std::vector<int> vec;
  const std::vector<std::byte> bytes = knot::serialize(vec);
  const auto expected = as_bytes({0, 0, 0, 0, 0, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  BOOST_CHECK(vec == knot::deserialize<std::vector<int>>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_ptr_iter) {
  const std::vector<std::byte> bytes = knot::serialize(5);
  BOOST_CHECK(5 == knot::deserialize<int>(bytes.data(), bytes.data() + bytes.size()));
}

BOOST_AUTO_TEST_CASE(serialize_map) {
  // maps are weird because value type has a const key: std::pair<const key, value>
  const std::map<Point, int> map{{Point{1, 1}, 5}, {Point{0, 0}, 8}};
  const std::vector<std::byte> bytes = knot::serialize(map);
  BOOST_CHECK(sizeof(std::size_t) + sizeof(Point) * 2 + sizeof(int) * 2 == bytes.size());

  const auto result = knot::deserialize<std::map<Point, int>>(bytes.begin(), bytes.end());
  BOOST_CHECK(map == result);
}

BOOST_AUTO_TEST_CASE(serialize_variant_point) {
  const std::variant<int, Point> var = Point{45, 89};
  const std::vector<std::byte> bytes = knot::serialize(var);
  const auto expected = as_bytes({1, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0, 89, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  const auto result = knot::deserialize<std::variant<int, Point>>(bytes.begin(), bytes.end());
  BOOST_CHECK(var == result);
}

BOOST_AUTO_TEST_CASE(serialize_variant_int) {
  const std::variant<int, Point> var = 5;
  const std::vector<std::byte> bytes = knot::serialize(var);
  const auto expected = as_bytes({0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0});
  BOOST_CHECK(expected == bytes);

  const auto result = knot::deserialize<std::variant<int, Point>>(bytes.begin(), bytes.end());
  BOOST_CHECK(var == result);
}

BOOST_AUTO_TEST_CASE(serialize_BigObject) {
  BigObject example = example_big_object();
  example.h = nullptr;  // unique_ptrs dont do deep comparison

  const std::vector<std::byte> bytes = knot::serialize(example);
  std::cout << "BigObject example serialized in " << bytes.size() << " bytes.\n";

  BOOST_CHECK(example == knot::deserialize<BigObject>(bytes.begin(), bytes.end()));
}

BOOST_AUTO_TEST_CASE(serialize_ByteTypes) {
  std::vector<uint8_t> buf_u8;
  knot::serialize(5, std::back_inserter(buf_u8));
  const auto result_u8 = knot::deserialize<int>(buf_u8.begin(), buf_u8.end());
  BOOST_CHECK(5 == result_u8);

  std::vector<int8_t> buf_i8;
  knot::serialize(5, std::back_inserter(buf_i8));
  const auto result_i8 = knot::deserialize<int>(buf_i8.begin(), buf_i8.end());
  BOOST_CHECK(5 == result_i8);

  std::vector<std::byte> buf_byte;
  knot::serialize(5, std::back_inserter(buf_byte));
  const auto result_byte = knot::deserialize<int>(buf_byte.begin(), buf_byte.end());
  BOOST_CHECK(5 == result_byte);
}

BOOST_AUTO_TEST_CASE(serialize_non_tuple_tie) {
  std::vector<std::byte> bytes = knot::serialize(IntWrapper{5});
  const auto result1 = knot::deserialize<IntWrapper>(bytes.begin(), bytes.end());
  BOOST_CHECK(IntWrapper{5} == result1);

  bytes = knot::serialize(VariantWrapper{5.0f});
  const auto result2 = knot::deserialize<VariantWrapper>(bytes.begin(), bytes.end());
  BOOST_CHECK(VariantWrapper{5.0f} == result2);

  bytes = knot::serialize(VecWrapper{{1, 2, 3}});
  const auto result3 = knot::deserialize<VecWrapper>(bytes.begin(), bytes.end());
  BOOST_CHECK((VecWrapper{{1, 2, 3}}) == result3);
}
