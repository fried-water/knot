#include "gtest/gtest.h"

#include "knot.h"

#include <variant>

namespace {

enum class PrimitiveType { Void, Char, Int, Float, Bool };

using PrimitiveValue = std::variant<std::tuple<>, char, int, float, bool>;

enum class BinaryOp { Add, Sub, Mul, Div, Eq, Neq, Gt, Ge, Lt, Le, And, Or };
enum class UnaryOp { Add, Sub, Not };

struct BinaryExpr;
struct UnaryExpr;
struct FunctionCall;

using Expr = std::variant<std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>, std::unique_ptr<FunctionCall>,
                          PrimitiveValue>;

struct BinaryExpr {
  BinaryOp op;
  Expr lhs;
  Expr rhs;
};

struct UnaryExpr {
  UnaryOp op;
  Expr child;
};

struct FunctionCall {
  std::string name;
  std::vector<Expr> args;
};

// TODO
// If
// Loop
// Scope
// FunctionDef
// VariableAssignment
//

template <typename T>
std::optional<PrimitiveType> typecheck(const T& expr) {
  struct TypeCheckVisitor {
    // Return types of leaf values
    std::optional<PrimitiveType> operator()(const PrimitiveValue& val) const {
      switch (val.index()) {
        case 0:
          return PrimitiveType::Void;
        case 1:
          return PrimitiveType::Char;
        case 2:
          return PrimitiveType::Int;
        case 3:
          return PrimitiveType::Float;
        case 4:
          return PrimitiveType::Bool;
        default:
          return std::nullopt;
      }
    }

    std::optional<PrimitiveType> operator()(const BinaryExpr& expr) const {
      std::optional<PrimitiveType> lhs = typecheck(expr.lhs);
      if (!lhs) return std::nullopt;
      std::optional<PrimitiveType> rhs = typecheck(expr.rhs);
      if (!rhs) return std::nullopt;
      if (*lhs != *rhs) return std::nullopt;  // No implicit type conversions

      const PrimitiveType type = *rhs;

      switch (expr.op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
          return (type == PrimitiveType::Char || type == PrimitiveType::Int || type == PrimitiveType::Float)
                     ? std::make_optional(type)
                     : std::nullopt;
        case BinaryOp::Gt:
        case BinaryOp::Ge:
        case BinaryOp::Lt:
        case BinaryOp::Le:
          return (type == PrimitiveType::Char || type == PrimitiveType::Int || type == PrimitiveType::Float)
                     ? std::make_optional(PrimitiveType::Bool)
                     : std::nullopt;
        case BinaryOp::And:
        case BinaryOp::Or:
          return type == PrimitiveType::Bool ? std::make_optional(type) : std::nullopt;
        case BinaryOp::Eq:
        case BinaryOp::Neq:
          return std::make_optional(PrimitiveType::Bool);
      }
    }

    std::optional<PrimitiveType> operator()(const UnaryExpr& expr) const {
      std::optional<PrimitiveType> child = typecheck(expr.child);
      if (!child) return std::nullopt;

      const PrimitiveType type = *child;

      switch (expr.op) {
        case UnaryOp::Add:
        case UnaryOp::Sub:
          return (type == PrimitiveType::Char || type == PrimitiveType::Int || type == PrimitiveType::Float)
                     ? std::make_optional(type)
                     : std::nullopt;
        case UnaryOp::Not:
          return type == PrimitiveType::Bool ? std::make_optional(type) : std::nullopt;
      }
    }

    std::optional<PrimitiveType> operator()(const FunctionCall&) const {
      // TODO
      return PrimitiveType::Void;
    }
  };

  return knot::evaluate<std::optional<PrimitiveType>>(expr, TypeCheckVisitor{});
}

Expr unary(UnaryOp op, Expr child) { return std::make_unique<UnaryExpr>(UnaryExpr{op, std::move(child)}); }

Expr binary(BinaryOp op, Expr lhs, Expr rhs) {
  return std::make_unique<BinaryExpr>(BinaryExpr{op, std::move(lhs), std::move(rhs)});
}

}  // namespace

TEST(AST, type_check_primitive) {
  EXPECT_EQ(PrimitiveType::Void, typecheck(PrimitiveValue{std::tuple()}));
  EXPECT_EQ(PrimitiveType::Char, typecheck(PrimitiveValue{'a'}));
  EXPECT_EQ(PrimitiveType::Int, typecheck(PrimitiveValue{1}));
  EXPECT_EQ(PrimitiveType::Float, typecheck(PrimitiveValue{1.0f}));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(PrimitiveValue{true}));
}

TEST(AST, type_check_unary) {
  EXPECT_EQ(PrimitiveType::Int, typecheck(unary(UnaryOp::Add, PrimitiveValue{1})));
  EXPECT_EQ(PrimitiveType::Int, typecheck(unary(UnaryOp::Sub, PrimitiveValue{1})));
  EXPECT_EQ(std::nullopt, typecheck(unary(UnaryOp::Not, PrimitiveValue{1})));

  EXPECT_EQ(std::nullopt, typecheck(unary(UnaryOp::Add, PrimitiveValue{true})));
  EXPECT_EQ(std::nullopt, typecheck(unary(UnaryOp::Sub, PrimitiveValue{true})));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(unary(UnaryOp::Not, PrimitiveValue{true})));
}

TEST(AST, type_check_binary) {
  EXPECT_EQ(PrimitiveType::Int, typecheck(binary(BinaryOp::Add, PrimitiveValue{1}, PrimitiveValue{1})));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(binary(BinaryOp::Gt, PrimitiveValue{1}, PrimitiveValue{1})));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(binary(BinaryOp::Eq, PrimitiveValue{1}, PrimitiveValue{1})));
  EXPECT_EQ(std::nullopt, typecheck(binary(BinaryOp::And, PrimitiveValue{1}, PrimitiveValue{1})));

  EXPECT_EQ(std::nullopt, typecheck(binary(BinaryOp::Add, PrimitiveValue{true}, PrimitiveValue{true})));
  EXPECT_EQ(std::nullopt, typecheck(binary(BinaryOp::Gt, PrimitiveValue{true}, PrimitiveValue{true})));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(binary(BinaryOp::Eq, PrimitiveValue{true}, PrimitiveValue{true})));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(binary(BinaryOp::And, PrimitiveValue{true}, PrimitiveValue{true})));
}
