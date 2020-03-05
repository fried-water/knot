#ifndef TEST_STRUCTS
#define TEST_STRUCTS

struct Point : knot::Compareable {
  int x = 0;
  int y = 0;
  Point() = default;
  Point(int x, int y) : x(x), y(y) {}
};
inline auto as_tie(const Point& p) { return std::tie(p.x, p.y); }

struct Bbox : knot::Compareable {
  Point min;
  Point max;
  Bbox() = default;
  Bbox(Point min, Point max) : min(min), max(max) {}
};
inline auto as_tie(const Bbox& b) { return std::tie(b.min, b.max); }

#endif
