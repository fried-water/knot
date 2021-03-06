# knot
Utility framework to recursively visit regular composite types. This allows knot to implement a generic hash(), serialize(), deserialize(), lexicographic ordered and comparison operators, debug() (without member names). This can already be done with most composite types in the standard library: tuple, pair, variant,  optional, most range containers, etc. However, due to a lack of reflection in the language its impossible to visit the members of a struct. Some libraries such as [boost work around this using macros.](https://www.boost.org/doc/libs/1_72_0/libs/fusion/doc/html/fusion/adapted/define_struct.html)

Instead knot require structs implement `as_tie(const T&)` that returns a tie of its members (or the subset that represent its identity). In c++17 `as_tie(const T&)` can be auto generated with some template hackery for aggregate structs with no base classes, otherwise it needs to be manually implemented. 

```cpp
struct Point {
  KNOT_COMPAREABLE(Point) // Generates comparison ops

  int x = 0;
  int y = 0;
};

void example(const Point& p) {
  std::size_t hash = knot::hash_value(p);
  std::cout << knot::debug(p) << '\n';

  std::vector<uint8_t> bytes = knot::serialize(p);
  assert(p == knot::deserialize<Point>(bytes.begin(), bytes.end()));
}
```

In addition to structs with `as_tie(const T&)`, knot also recusively supports most composite types in the standard language: tuple, pair, variant, optional, any range, unique/shared/raw ptr. In addition to the above functions knot also provides a higher level `preorder(const T&, F)` and `preorder_accumulate<Acc>(const T&, F, Acc)` function that traverse objects in a preorder traversal.

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

int num_ops(const Expr& expr, const Op desired_op) {
  return knot::preorder_accumulate(expr, [desired_op](int acc, Op op) {
    return op == desired_op ? acc + 1 : acc;
  }, 0);
}

void dump_leaf_values(const Expr& expr) {
  return knot::preorder(expr, [](int leaf) {
    std::cout << "Leaf: " << leaf << '\n';
  });
}

```
