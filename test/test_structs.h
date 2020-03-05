#ifndef TEST_STRUCTS
#define TEST_STRUCTS

struct Point {
  int x;
  int y;
};
inline auto as_tie(const Point& p) { return std::tie(p.x, p.y); }

struct Bbox {
  Point min;
  Point max;
};
inline auto as_tie(const Bbox& b) { return std::tie(b.min, b.max); }

#endif
