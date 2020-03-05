#ifndef TEST_STRUCTS
#define TEST_STRUCTS

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

struct Point : knot::Ordered {
  int x = 0;
  int y = 0;
  Point() = default;
  Point(int x, int y) : x(x), y(y) {}
};
inline auto as_tie(const Point& p) { return std::tie(p.x, p.y); }

struct Bbox : knot::Ordered {
  Point min;
  Point max;
  Bbox() = default;
  Bbox(Point min, Point max) : min(min), max(max) {}
};
inline auto as_tie(const Bbox& b) { return std::tie(b.min, b.max); }

// BigObject can't be ordered since unordered containers dont have order operators
struct BigObject : knot::Compareable {
  std::set<Bbox> a;
  std::unordered_set<Bbox, knot::Hash> b;
  std::map<Bbox, int> c;
  std::unordered_map<Bbox, int, knot::Hash> d;
  std::optional<Bbox> e;
  std::tuple<Bbox, int> f;
  std::array<Bbox, 2> g;
  std::unique_ptr<Bbox> h;
  std::variant<int, Bbox> i;

  BigObject() = default;
  BigObject(std::set<Bbox> a, std::unordered_set<Bbox, knot::Hash> b, std::map<Bbox, int> c,
            std::unordered_map<Bbox, int, knot::Hash> d, std::optional<Bbox> e, std::tuple<Bbox, int> f,
            std::array<Bbox, 2> g, std::unique_ptr<Bbox> h, std::variant<int, Bbox> i)
      : a{std::move(a)},
        b{std::move(b)},
        c{std::move(c)},
        d{std::move(d)},
        e{std::move(e)},
        f{std::move(f)},
        g{std::move(g)},
        h{std::move(h)},
        i{std::move(i)} {}
};

inline auto as_tie(const BigObject& b) { return std::tie(b.a, b.b, b.c, b.d, b.e, b.f, b.g, b.h, b.i); }

#endif
