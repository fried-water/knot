#include "knot.h"

#include <iostream>

enum class Op { Add, Sub };

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
  return knot::accumulate(expr,
                          [](const auto& value, int count) {
                            if constexpr (std::is_same_v<Op, std::decay_t<decltype(value)>>) {
                              return value == Op::Add ? count + 1 : count;
                            }
                            return count;
                          },
                          0);
}

void dump_leaf_values(const Expr& expr) {
  return knot::visit(expr, [](const auto& value) {
    if constexpr (std::is_same_v<int, std::decay_t<decltype(value)>>) {
      std::cout << "Leaf: " << value << '\n';
    }
  });
}

int expr_main() {
  Expr e1 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Add, Expr{5}, Expr{7}});
  Expr e2 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, Expr{8}, Expr{2}});
  Expr e3 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, std::move(e2), Expr{4}});
  Expr combined = std::make_unique<BinaryExpr>(BinaryExpr{Op::Add, std::move(e1), std::move(e3)});

  std::cout << "Add ops: " << num_add_ops(combined) << "\n";
  dump_leaf_values(combined);

  example(combined);
}
/*
Add ops: 2
Leaf: 5
Leaf: 7
Leaf: 8
Leaf: 2
Leaf: 4
<<(0, <<(0, <5>, <7>)>>, <<(1, <<(1, <8>, <2>)>>, <4>)>>)>>
*/