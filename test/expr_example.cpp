#include "gtest/gtest.h"

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

int num_ops(const Expr& expr, const Op op) {
  return knot::accumulate(expr,
                          [op](const auto& value, int count) {
                            if constexpr (std::is_same_v<Op, std::decay_t<decltype(value)>>) {
                              return value == op ? count + 1 : count;
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

struct EvalVisitor {
  int operator()(const BinaryExpr& expr, int op, int lhs, int rhs) const {
    return expr.op == Op::Add ? lhs + rhs : lhs - rhs;
  }

  int operator()(Op op) const { return 0; }
};

TEST(Expr, test) {
  Expr e1 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, Expr{5}, Expr{7}});
  Expr e2 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, Expr{8}, Expr{2}});
  Expr e3 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, std::move(e2), Expr{4}});
  const Expr expr = std::make_unique<BinaryExpr>(BinaryExpr{Op::Add, std::move(e1), std::move(e3)});

  EXPECT_EQ(1, num_ops(expr, Op::Add));
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

TEST(Expr, eval) {
  Expr e1 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, Expr{5}, Expr{7}});
  Expr e2 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, Expr{8}, Expr{2}});
  Expr e3 = std::make_unique<BinaryExpr>(BinaryExpr{Op::Sub, std::move(e2), Expr{4}});
  const Expr expr = std::make_unique<BinaryExpr>(BinaryExpr{Op::Add, std::move(e1), std::move(e3)});

  EXPECT_EQ(0, knot::evaluate<int>(expr, EvalVisitor{}));
}
