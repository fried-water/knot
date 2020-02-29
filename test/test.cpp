#include "knot.h"

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

enum class Day { Mon, Tue, Wed, Thu, Fri, Sat, Sun };

struct X : knot::Ordered {
  std::optional<int> a;
  std::string b;
  std::vector<int> c;
  Day d;

  X() = default;
  X(std::optional<int> a, std::string b, std::vector<int> c, Day d) : a{a}, b{std::move(b)}, c{std::move(c)}, d{d} {}
};

// Y can't be ordered since unordered containers dont have order operators
struct Y : knot::Compareable {
  std::set<X> a;
  std::unordered_set<X, knot::Hash> b;
  std::map<int, X> c;
  std::unordered_map<int, X, knot::Hash> d;
  std::optional<X> e;
  std::tuple<X, int> f;
  std::array<X, 2> g;
  std::unique_ptr<X> h;
  std::variant<int, X> i;

  Y() = default;
  Y(std::set<X> a, std::unordered_set<X, knot::Hash> b, std::map<int, X> c, std::unordered_map<int, X, knot::Hash> d,
    std::optional<X> e, std::tuple<X, int> f, std::array<X, 2> g, std::unique_ptr<X> h, std::variant<int, X> i)
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

auto as_tie(const X& x) { return std::tie(x.a, x.b, x.c, x.d); }
auto as_tie(const Y& y) { return std::tie(y.a, y.b, y.c, y.d, y.e, y.f, y.g, y.h, y.i); }

template <typename, typename = void>
struct is_streamable : std::false_type {};
template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>> : std::true_type {};

int main() {
  X x{5, "abc", {3, 4, 5}, Day::Tue};
  Y y;
  y.a.insert(x);
  y.b.insert(x);
  y.c[3] = x;
  y.d[2] = x;
  y.f = std::make_tuple(X{}, 4);
  y.g = {X{}, x};
  y.h = std::make_unique<X>(X{});
  y.i = x;

  std::unordered_set<Y, knot::Hash> uset;  // Y can go in an unordered_set
  uset.insert(Y{});

  std::cout << "X: " << knot::debug_string(x) << "\n";
  std::cout << "Y: " << knot::debug_string(y) << "\n";

  std::cout << "X hash: " << knot::hash_value(x) << "\n";
  std::cout << "Y hash: " << knot::hash_value(y) << "\n";

  std::vector<uint8_t> x_bytes = knot::serialize(x);
  std::vector<uint8_t> y_bytes = knot::serialize(y);

  std::cout << "Serialization X size: " << x_bytes.size() << "\n";
  std::cout << "Serialization Y size: " << y_bytes.size() << "\n";

  assert(x == knot::deserialize<X>(x_bytes.begin(), x_bytes.end()));
  Y y2 = *knot::deserialize<Y>(y_bytes.begin(), y_bytes.end());

  const int num_threes = knot::accumulate<int>(y, [](const auto& t, int count) {
    if constexpr (std::is_arithmetic_v<std::decay_t<decltype(t)>>) {
      return t == 3 ? count + 1 : count;
    }
    return count;
  });
  std::cout << "Y has " << num_threes << " threes\n";

  const std::vector<X> xs = knot::accumulate<std::vector<X>>(y, [](const auto& t, std::vector<X> vec) {
    if constexpr (std::is_same_v<X, std::decay_t<decltype(t)>>) {
      vec.push_back(t);
    }
    return vec;
  });
  std::cout << "Y has " << xs.size() << " Xs\n";

  knot::visit(x, [](const auto& t) {
    if constexpr (is_streamable<std::decay_t<decltype(t)>>::value) {
      std::cout << typeid(t).name() << ": " << t << "\n";
      return false;
    } else {
      std::cout << typeid(t).name() << "\n";
      return true;
    }
  });

  // unique_ptr doesnt do deep equality comparison
  assert(y != y2);
  assert(*y2.h == *y.h);
  y.h = nullptr;
  y2.h = nullptr;
  assert(y == y2);
}