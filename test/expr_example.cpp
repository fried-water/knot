#include "gtest/gtest.h"

#include "knot.h"

#include <iostream>

enum class Op { Add, Sub };

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

int num_ops(const Expr& expr, const Op desired_op) {
  return knot::accumulate(expr, [desired_op](Op op, int count) { return op == desired_op ? count + 1 : count; }, 0);
}

void dump_leaf_values(const Expr& expr) {
  return knot::visit(expr, [](int leaf) { std::cout << "Leaf: " << leaf << '\n'; });
}

int eval(const Expr& expr) {
  struct EvalVisitor {
    int operator()(const BinaryExpr& expr, int lhs, int rhs) const {
      return expr.op == Op::Add ? lhs + rhs : lhs - rhs;
    }
    int operator()(const UnaryExpr& expr, int child) const { return expr.op == Op::Add ? child : -child; }
  };
  return knot::evaluate<int>(expr, EvalVisitor{});
}

Expr binary(Op op, Expr lhs, Expr rhs) {
  return std::make_unique<BinaryExpr>(BinaryExpr{op, std::move(lhs), std::move(rhs)});
}
Expr unary(Op op, Expr child) { return std::make_unique<UnaryExpr>(UnaryExpr{op, std::move(child)}); }

// ((5 - 7) + (-(8 + 2) - 4))
// = -2 + (-10 - 4)
// = -16
auto make_big_expr() {
  Expr e1 = binary(Op::Sub, Expr{5}, Expr{7});
  Expr e2 = unary(Op::Sub, binary(Op::Add, Expr{8}, Expr{2}));
  Expr e3 = binary(Op::Sub, std::move(e2), Expr{4});
  return binary(Op::Add, std::move(e1), std::move(e3));
}

TEST(Expr, test) {
  const Expr expr = make_big_expr();

  EXPECT_EQ(2, num_ops(expr, Op::Add));
  EXPECT_EQ(3, num_ops(expr, Op::Sub));

  dump_leaf_values(expr);
  std::cout << "Hash: " << knot::hash_value(expr) << '\n';
  std::cout << knot::debug_string(expr) << '\n';

  const std::vector<uint8_t> bytes = knot::serialize(expr);
  const std::optional<Expr> deserialized = knot::deserialize<Expr>(bytes.begin(), bytes.end());

  EXPECT_TRUE(deserialized.has_value());
  // Don't have op== and unique_ptr doesn't do deep comparisons anyway so compare strings instead
  EXPECT_EQ(knot::debug_string(expr), knot::debug_string(*deserialized));
}

TEST(Expr, eval) { EXPECT_EQ(-16, eval(make_big_expr())); }
