#include "gtest/gtest.h"

#include "knot.h"

#include <iostream>

namespace {

enum class Op { Add, Sub };

std::ostream& debug(std::ostream& os, Op op) {
  return os << (op == Op::Add ? "add" : "sub");
}

struct BinaryExpr;
struct UnaryExpr;

using Expr = std::variant<std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>, int>;

struct BinaryExpr {
  Op op;
  Expr lhs;
  Expr rhs;

  friend auto names(knot::Type<BinaryExpr>) { return knot::Names{"BinaryExpr", {"op", "lhs", "rhs"}}; }
};

struct UnaryExpr {
  Op op;
  Expr child;

  friend auto names(knot::Type<UnaryExpr>) { return knot::Names{"UnaryExpr", {"op", "child"}}; }
};

int num_ops(const Expr& expr, const Op desired_op) {
  return knot::preorder_accumulate(
      expr, [desired_op](int acc, Op op) { return op == desired_op ? acc + 1 : acc; }, 0);
}

void dump_leaf_values(const Expr& expr) {
  return knot::preorder(expr, [](int leaf) { std::cout << "Leaf: " << leaf << '\n'; });
}

Expr binary(Op op, Expr lhs, Expr rhs) {
  return std::make_unique<BinaryExpr>(BinaryExpr{op, std::move(lhs), std::move(rhs)});
}

Expr unary(Op op, Expr child) { return std::make_unique<UnaryExpr>(UnaryExpr{op, std::move(child)}); }

auto make_big_expr() {
  // (5 - 7) + (-(8 + 2) - 4)
  // = -2 + (-10 - 4)
  // = -16
  return binary(Op::Add, binary(Op::Sub, 5, 7), binary(Op::Sub, unary(Op::Sub, binary(Op::Add, 8, 2)), 4));
}

}  // namespace

TEST(Expr, test) {
  const Expr expr = make_big_expr();

  EXPECT_EQ(2, num_ops(expr, Op::Add));
  EXPECT_EQ(3, num_ops(expr, Op::Sub));

  dump_leaf_values(expr);
  std::cout << "Hash: " << knot::hash_value(expr) << '\n';
  std::cout << knot::debug(expr) << '\n';

  const std::vector<std::byte> bytes = knot::serialize(expr);
  const std::optional<Expr> deserialized = knot::deserialize<Expr>(bytes.begin(), bytes.end());

  EXPECT_TRUE(deserialized.has_value());
  // Don't have op== and unique_ptr doesn't do deep comparisons anyway so compare strings instead
  EXPECT_EQ(knot::debug(expr), knot::debug(*deserialized));
}
