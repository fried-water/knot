#include "gtest/gtest.h"

#include "knot.h"
#include "test_structs.h"

#include <map>

namespace {

std::vector<std::byte> as_bytes(std::initializer_list<uint8_t> chars) {
  std::vector<std::byte> bytes;
  std::transform(chars.begin(), chars.end(), std::back_inserter(bytes), [](uint8_t c) { return std::byte{c}; });

  return bytes;
}

}  // namespace

TEST(Serialize, primitive) {
  const std::vector<std::byte> bytes = knot::serialize(5);
  // Does this depend on endianess of machine?
  const auto expected = as_bytes({5, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(5, knot::deserialize<int>(bytes.begin(), bytes.end()));
}

TEST(Serialize, not_enough_bytes) {
  const std::vector<uint8_t> empty_bytes;
  EXPECT_EQ(std::nullopt, knot::deserialize<int>(empty_bytes.begin(), empty_bytes.end()));
}

TEST(Serialize, extra_bytes) {
  const std::vector<uint8_t> extra_bytes{5, 0, 0, 0, 0};
  EXPECT_EQ(std::nullopt, knot::deserialize<int>(extra_bytes.begin(), extra_bytes.end()));
}

TEST(Serialize, basic_struct) {
  const Point p{45, 89};
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({45, 0, 0, 0, 89, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(p, knot::deserialize<Point>(bytes.begin(), bytes.end()));
}

TEST(Serialize, composite_struct) {
  const Bbox bbox{Point{1, 2}, Point{3, 4}};
  const std::vector<std::byte> bytes = knot::serialize(bbox);
  EXPECT_EQ(16, bytes.size());

  EXPECT_EQ(bbox, knot::deserialize<Bbox>(bytes.begin(), bytes.end()));
}

TEST(Serialize, optional) {
  const std::optional<Point> p = Point{45, 89};
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({1, 45, 0, 0, 0, 89, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(p, knot::deserialize<std::optional<Point>>(bytes.begin(), bytes.end()));
}

TEST(Serialize, nullopt) {
  const std::optional<Point> p;
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(p, *knot::deserialize<std::optional<Point>>(bytes.begin(), bytes.end()));
}

TEST(Serialize, unique_ptr) {
  const auto p = std::make_unique<Point>(Point{45, 89});
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({1, 45, 0, 0, 0, 89, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(*p, **knot::deserialize<std::unique_ptr<Point>>(bytes.begin(), bytes.end()));
}

TEST(Serialize, null_unique_ptr) {
  const std::unique_ptr<Point> p;
  const std::vector<std::byte> bytes = knot::serialize(p);
  const auto expected = as_bytes({0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(p, knot::deserialize<std::unique_ptr<Point>>(bytes.begin(), bytes.end()));
}

TEST(Serialize, range) {
  const std::vector<int> vec{1, 2, 3};
  const std::vector<std::byte> bytes = knot::serialize(vec);
  const auto expected = as_bytes({3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(vec, knot::deserialize<std::vector<int>>(bytes.begin(), bytes.end()));
}

TEST(Serialize, empty_range) {
  const std::vector<int> vec;
  const std::vector<std::byte> bytes = knot::serialize(vec);
  const auto expected = as_bytes({0, 0, 0, 0, 0, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  EXPECT_EQ(vec, knot::deserialize<std::vector<int>>(bytes.begin(), bytes.end()));
}

TEST(Serialize, map) {
  // maps are weird because value type has a const key: std::pair<const key, value>
  const std::map<Point, int> map{{Point{1, 1}, 5}, {Point{0, 0}, 8}};
  const std::vector<std::byte> bytes = knot::serialize(map);
  EXPECT_EQ(sizeof(std::size_t) + sizeof(Point) * 2 + sizeof(int) * 2, bytes.size());

  const auto result = knot::deserialize<std::map<Point, int>>(bytes.begin(), bytes.end());
  EXPECT_EQ(map, result);
}

TEST(Serialize, variant_point) {
  const std::variant<int, Point> var = Point{45, 89};
  const std::vector<std::byte> bytes = knot::serialize(var);
  const auto expected = as_bytes({1, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0, 89, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  const auto result = knot::deserialize<std::variant<int, Point>>(bytes.begin(), bytes.end());
  EXPECT_EQ(var, result);
}

TEST(Serialize, variant_int) {
  const std::variant<int, Point> var = 5;
  const std::vector<std::byte> bytes = knot::serialize(var);
  const auto expected = as_bytes({0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0});
  EXPECT_EQ(expected, bytes);

  const auto result = knot::deserialize<std::variant<int, Point>>(bytes.begin(), bytes.end());
  EXPECT_EQ(var, result);
}

TEST(Serialize, BigObject) {
  BigObject example = example_big_object();
  example.h = nullptr;  // unique_ptrs dont do deep comparison

  const std::vector<std::byte> bytes = knot::serialize(example);
  std::cout << "BigObject example serialized in " << bytes.size() << " bytes.\n";

  EXPECT_EQ(example, knot::deserialize<BigObject>(bytes.begin(), bytes.end()));
}

TEST(Serialize, ByteTypes) {
  std::vector<uint8_t> buf_u8;
  knot::serialize(5, std::back_inserter(buf_u8));
  const auto result_u8 = knot::deserialize<int>(buf_u8.begin(), buf_u8.end());
  EXPECT_EQ(5, result_u8);

  std::vector<int8_t> buf_i8;
  knot::serialize(5, std::back_inserter(buf_i8));
  const auto result_i8 = knot::deserialize<int>(buf_i8.begin(), buf_i8.end());
  EXPECT_EQ(5, result_i8);

  std::vector<std::byte> buf_byte;
  knot::serialize(5, std::back_inserter(buf_byte));
  const auto result_byte = knot::deserialize<int>(buf_byte.begin(), buf_byte.end());
  EXPECT_EQ(5, result_byte);
}
