# knot
Utility framework to recursively visit regular composite types. This allows knot to implement a generic hash(), serialize(), deserialize(), lexicographic ordered and comparison operators, debug_string() (without member names). This can already be done with most composite types in the standard library: tuple, pair, variant,  optional, most range containers, etc. However, due to a lack of reflection in the language its impossible to visit the members of a struct. Some libraries such as [boost work around this using macros.](https://www.boost.org/doc/libs/1_72_0/libs/fusion/doc/html/fusion/adapted/define_struct.html)

Instead knot require structs implement `as_tie(const T&)` that returns a tie of its members (or the subset that represent its identity).

```cpp
struct Point : knot::Compareable {
  int x = 0;
  int y = 0;
  Point() = default;
  Point(int x, int y) : x(x), y(y) {}
};

auto as_tie(const Point& p) { return std::tie(p.x, p.y); }

void example(const Point& p) {
  std::size_t hash = knot::hash_value(p);
  std::cout << knot::debug_string(p) << '\n';

  std::vector<uint8_t> bytes = knot::serialize(p);
  assert(p == knot::deserialize<Point>(bytes.begin(), bytes.end()));
}
```

In addition to structs with as_tie, knot also recusively supports most composite types in the standard language: tuple, pair, variant, optional, any range, unique/shared/raw ptr. In addition to the above functions knot also provides a higher level `visit()` and `accumulate()` function that traverse objects in a preorder traversal.

```cpp
enum class Op {Add, Sub};

struct BinaryExpr;

using Expr = std::variant<std::unique_ptr<BinaryExpr>, int>;

struct BinaryExpr {
  Op op;
  Expr lhs;
  Expr rhs;
};

auto as_tie(const BinaryExpr& b) { return std::tie(b.op, b.lhs, b.rhs); }

void example(const Expr& expr) {
  std::size_t hash = knot::hash_value(expr);
  std::cout << knot::debug_string(expr) << '\n';

  std::vector<uint8_t> bytes = knot::serialize(expr);
  std::optional<Expr> expr2 = knot::deserialize<Expr>(bytes.begin(), bytes.end());
}

int num_add_ops(const Expr& expr) {
  return knot::accumulate(expr, [](const auto& value, int count) {
      if constexpr (std::is_same_v<Op, std::decay_t<decltype(value)>>) {
        return value == Op::Add ? count + 1 : count;
      }
      return count;
    }, 0);
}

void dump_leaf_values(const Expr& expr) {
  return knot::visit(expr, [](const auto& value) {
      if constexpr (std::is_same_v<int, std::decay_t<decltype(value)>>) {
        std::cout << "Leaf: " << value << '\n';
      }
    });
}
```
