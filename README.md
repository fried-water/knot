# knot

knot is a cpp++17 dependency free header only library to provide generic utility functions such as hash_value(), serialize(), debug() (without member names), lexicographic order and comparison operators (superseded by spaceship operator in c++20) to aggregate types with no base classes. knot also supports most std containers (tuple, pair, optional, variant, most range containers). The original motivation was `#[derive(Debug)]` from rust and similar.

```cpp
struct Point {
  int x = 0;
  int y = 0;

  KNOT_COMPAREABLE(Point) // Generates comparison ops
};

void example(const Point& p) {
  std::size_t hash = knot::hash_value(p);
  std::cout << knot::debug(p) << '\n';
  // Eg: "(1, 2)"

  // should serialize to 8 bytes
  std::vector<uint8_t> bytes = knot::serialize(p);
  assert(p == knot::deserialize<Point>(bytes.begin(), bytes.end()));
}
```

Many of the above operations are implemented in terms of more generic static traversals. For example if you consider `std::pair<std::string, std::optional<int>>` as depth 3 tree with pair at the root, string and optional as its children, and the strings characters and int as the leaves. `knot::preorder(const T&)` will statically visit this tree in a preorder traversal, likewise with `knot::postorder(const T&)`. `knot::visit(const T&)` will visit the direct children, same as std::visit for variant, the members of a struct, or the elements of a range. All the traveral functions will statically skip visiting objects if the supplied visitor cannot be called, be wary of implicit conversions.

```cpp
enum class Op {Add, Sub};

struct BinaryExpr;
struct UnaryExpr;

using Expr = std::variant<std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>, int>;

struct BinaryExpr {
  Op op;
  Expr lhs;
  Expr rhs;
};

struct UnaryExpr {
  Op op;
  Expr child;
};

void example(const Expr& expr) {
  std::size_t hash = knot::hash_value(expr);
  std::cout << knot::debug(expr) << '\n';

  std::vector<uint8_t> bytes = knot::serialize(expr);
  std::optional<Expr> deserialized = knot::deserialize<Expr>(bytes.begin(), bytes.end());

  // Don't have op== and unique_ptr doesn't do deep comparisons anyway so compare strings instead
  assert(deserialized.has_value() && knot::debug(expr) == knot::debug(*deserialized));
}

// Count the number of Add operators in the expression tree
int num_adds(const Expr& expr) {
  return knot::preorder_accumulate(expr, [](int acc, Op op) {
    return op == Op::Add ? acc + 1 : acc;
  }, 0);
}

// Print all leaf integers in the expression tree
void dump_leaf_values(const Expr& expr) {
  return knot::preorder(expr, [](int leaf) {
    std::cout << "Leaf: " << leaf << '\n';
  });
}

```

Look at the unit tests under test/ for more examples.
