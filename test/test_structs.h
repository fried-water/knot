#ifndef TEST_STRUCTS
#define TEST_STRUCTS

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

struct Point {
  KNOT_ORDERED(Point)
  int x = 0;
  int y = 0;
};

struct Bbox {
  KNOT_ORDERED(Bbox)
  Point min;
  Point max;
};

// BigObject can't be ordered since unordered containers dont have order operators
struct BigObject {
  KNOT_COMPAREABLE(BigObject)

  std::set<Bbox> a;
  std::unordered_set<Bbox, knot::Hash> b;
  std::map<Bbox, int> c;
  std::unordered_map<Bbox, int, knot::Hash> d;
  std::optional<Bbox> e;
  std::tuple<Bbox, int> f;
  std::array<Bbox, 2> g;
  std::unique_ptr<Bbox> h;
  std::variant<int, Bbox> i;
};

inline BigObject example_big_object() {
  const Bbox small_box{Point{0, 0}, Point{1, 1}};
  const Bbox big_box{Point{0, 0}, Point{50, 50}};

  BigObject obj;
  obj.a.insert(small_box);
  obj.b.insert(big_box);
  obj.c[small_box] = 5;
  obj.d[big_box] = 6;
  obj.e = small_box;
  obj.f = std::make_tuple(big_box, 5);
  obj.g = {small_box, big_box};
  obj.h = std::make_unique<Bbox>(small_box);
  obj.i = big_box;

  return obj;
}

#endif
