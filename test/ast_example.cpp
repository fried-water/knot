#include "gtest/gtest.h"

#include "knot.h"

#include <variant>

namespace {

enum class PrimitiveType { Void, Char, Int, Float, Bool };

struct Void {};
struct Char { char val; };
struct Int { int val; };
struct Float { float val; };
struct Bool { bool val; };

using PrimitiveValue = std::variant<Void, Char, Int, Float, Bool>;

struct BinaryAddExpr;
struct BinarySubExpr;
struct BinaryMulExpr;
struct BinaryDivExpr;
struct BinaryGtExpr;
struct BinaryGeExpr;
struct BinaryLtExpr;
struct BinaryLeExpr;
struct BinaryEqExpr;
struct BinaryNeqExpr;

struct UnaryPlusExpr;
struct UnaryMinusExpr;
struct UnaryNegateExpr;

struct FunctionCall;

struct IfExpr;

using Expr = std::variant<
  std::unique_ptr<BinaryAddExpr>,
  std::unique_ptr<BinarySubExpr>,
  std::unique_ptr<BinaryMulExpr>,
  std::unique_ptr<BinaryDivExpr>,
  std::unique_ptr<BinaryGtExpr>,
  std::unique_ptr<BinaryGeExpr>,
  std::unique_ptr<BinaryLtExpr>,
  std::unique_ptr<BinaryLeExpr>,
  std::unique_ptr<BinaryEqExpr>,
  std::unique_ptr<BinaryNeqExpr>,
  std::unique_ptr<UnaryPlusExpr>,
  std::unique_ptr<UnaryMinusExpr>,
  std::unique_ptr<UnaryNegateExpr>,
  std::unique_ptr<FunctionCall>,
  std::unique_ptr<IfExpr>,
  PrimitiveValue>;

struct BinaryAddExpr { Expr lhs; Expr rhs; };
struct BinarySubExpr { Expr lhs; Expr rhs; };
struct BinaryMulExpr { Expr lhs; Expr rhs; };
struct BinaryDivExpr { Expr lhs; Expr rhs; };
struct BinaryGtExpr { Expr lhs; Expr rhs; };
struct BinaryGeExpr { Expr lhs; Expr rhs; };
struct BinaryLtExpr { Expr lhs; Expr rhs; };
struct BinaryLeExpr { Expr lhs; Expr rhs; };
struct BinaryEqExpr { Expr lhs; Expr rhs; };
struct BinaryNeqExpr { Expr lhs; Expr rhs; };
struct BinaryAndExpr { Expr lhs; Expr rhs; };
struct BinaryOrExpr { Expr lhs; Expr rhs; };

struct UnaryPlusExpr { Expr child; };
struct UnaryMinusExpr { Expr child; };
struct UnaryNegateExpr { Expr child; };

struct FunctionCall {
  std::string name;
  std::vector<Expr> args;
};

struct Scope {
  // TODO
  // std::vector<Assignment> variables;
  Expr expr;
};

struct IfExpr {
  Expr if_cond;
  Scope if_body;
  std::vector<std::tuple<Expr, Scope>> elsifs;

  // TODO figure out why scope won't work in an std::optional
  std::optional<Expr> else_body;
};

struct FunctionDef {
  std::string name;
  PrimitiveType result;
  std::vector<std::string> arg_names;
  std::vector<PrimitiveType> args;
  Scope body;
};

struct Root {
  std::vector<FunctionDef> functions;
};

// TODO
// Loop
// VariableAssignment

bool is_number(const PrimitiveType t) {
  return t == PrimitiveType::Char || t == PrimitiveType::Int || t == PrimitiveType::Float;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
std::optional<PrimitiveType> typecheck(const T& root) {
  using OptType = std::optional<PrimitiveType>;

  auto funcs = knot::accumulate(root, [](std::unordered_map<std::string, const FunctionDef*> funcs, const FunctionDef& def) {
    funcs[def.name] = &def;
    return funcs;
  }, std::unordered_map<std::string, const FunctionDef*>{});

  return knot::evaluate<OptType>(root, overloaded {
    // Return types of leaf values
    [](Void) { return PrimitiveType::Void; },
    [](Char) { return PrimitiveType::Char; },
    [](Int) { return PrimitiveType::Int; },
    [](Float) { return PrimitiveType::Float; },
    [](Bool) { return PrimitiveType::Bool; },

    [](const BinaryAddExpr&, OptType lhs, OptType rhs) {
      return lhs && is_number(*lhs) && (lhs == rhs)  ? lhs : std::nullopt;
    },
    [](const BinarySubExpr&, OptType lhs, OptType rhs) {
      return lhs && is_number(*lhs) && (lhs == rhs)  ? lhs : std::nullopt;
    },
    [](const BinaryMulExpr&, OptType lhs, OptType rhs) {
      return lhs && is_number(*lhs) && (lhs == rhs)  ? lhs : std::nullopt;
    },
    [](const BinaryDivExpr&, OptType lhs, OptType rhs) {
      return lhs && is_number(*lhs) && (lhs == rhs)  ? lhs : std::nullopt;
    },

    [](const BinaryGtExpr&, OptType lhs, OptType rhs) {
      return lhs && (lhs == rhs) ? OptType(PrimitiveType::Bool) : std::nullopt;
    },
    [](const BinaryGeExpr&, OptType lhs, OptType rhs) {
      return lhs && (lhs == rhs) ? OptType(PrimitiveType::Bool) : std::nullopt;
    },
    [](const BinaryLtExpr&, OptType lhs, OptType rhs) {
      return lhs && (lhs == rhs) ? OptType(PrimitiveType::Bool) : std::nullopt;
    },
    [](const BinaryLeExpr&, OptType lhs, OptType rhs) {
      return lhs && (lhs == rhs) ? OptType(PrimitiveType::Bool) : std::nullopt;
    },
    [](const BinaryEqExpr&, OptType lhs, OptType rhs) {
      return lhs && (lhs == rhs) ? OptType(PrimitiveType::Bool) : std::nullopt;
    },
    [](const BinaryNeqExpr&, OptType lhs, OptType rhs) {
      return lhs && (lhs == rhs) ? OptType(PrimitiveType::Bool) : std::nullopt;
    },

    [](const BinaryAndExpr&, OptType lhs, OptType rhs) {
      return lhs == PrimitiveType::Bool && rhs == PrimitiveType::Bool? lhs : std::nullopt;
    },
    [](const BinaryOrExpr&, OptType lhs, OptType rhs) {
      return lhs == PrimitiveType::Bool && rhs == PrimitiveType::Bool? lhs : std::nullopt;
    },

    [](const UnaryPlusExpr&, OptType child) {
      return child && is_number(*child) ? child : std::nullopt;
    },
    [](const UnaryMinusExpr&, OptType child) {
      return child && is_number(*child) ? child : std::nullopt;
    },
    [](const UnaryNegateExpr&, OptType child) {
      return child == PrimitiveType::Bool ? child : std::nullopt;
    },
    [](const IfExpr&, OptType if_cond, OptType if_body, std::vector<std::tuple<OptType, OptType>> elsifs, std::optional<OptType> else_body) {
      const bool args_are_bool = if_cond == PrimitiveType::Bool && 
        std::all_of(elsifs.begin(), elsifs.end(), [](const auto& tup) { return std::get<0>(tup) == PrimitiveType::Bool; });
      
      const bool scopes_not_none = if_body != std::nullopt && 
        (else_body == std::nullopt || *else_body != std::nullopt) && 
        std::all_of(elsifs.begin(), elsifs.end(), [](const auto& tup) { return std::get<1>(tup) != std::nullopt; });

      return args_are_bool && scopes_not_none ? OptType(PrimitiveType::Void) : std::nullopt;
    },

    [&funcs](const FunctionCall& call, const auto& eval) -> OptType {
      auto it = funcs.find(call.name);
      if(it == funcs.end()) return std::nullopt;

      const FunctionDef* def = it->second;
      const std::vector<OptType> call_types = eval(call.args);
      return std::equal(call_types.begin(), call_types.end(), def->args.begin(), def->args.end())
        ? OptType(def->result)
        : std::nullopt;
    },

    [](const FunctionDef& def, const auto& eval) -> OptType {
      return eval(def.body) == def.result ? OptType(def.result) : std::nullopt;
    },

    [](const Scope&, OptType body) { return body; },
    [](const Root&, std::vector<OptType> funcs) {
      return std::all_of(funcs.begin(), funcs.end(), [](const auto& opt) { return static_cast<bool>(opt); })
        ? OptType(PrimitiveType::Void)
        : std::nullopt;
    }
  });
}

auto unary_plus(PrimitiveValue val) { return std::make_unique<UnaryPlusExpr>(UnaryPlusExpr{val}); }
auto unary_minus(PrimitiveValue val) { return std::make_unique<UnaryMinusExpr>(UnaryMinusExpr{val}); }
auto unary_negate(PrimitiveValue val) { return std::make_unique<UnaryNegateExpr>(UnaryNegateExpr{val}); }

}  // namespace

auto check_root(const Root& root) { return typecheck(root); }

TEST(AST, type_check_primitive) {
  EXPECT_EQ(PrimitiveType::Void, typecheck(Void{}));
  EXPECT_EQ(PrimitiveType::Char, typecheck(Char{'a'}));
  EXPECT_EQ(PrimitiveType::Int, typecheck(Int{1}));
  EXPECT_EQ(PrimitiveType::Float, typecheck(Float{1.0f}));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(Bool{true}));
}

TEST(AST, type_check_unary) {
  EXPECT_EQ(PrimitiveType::Int, typecheck(unary_plus(PrimitiveValue{Int{1}})));
  EXPECT_EQ(PrimitiveType::Int, typecheck(unary_minus(PrimitiveValue{Int{1}})));
  EXPECT_EQ(std::nullopt, typecheck(unary_negate(PrimitiveValue{Int{1}})));

  EXPECT_EQ(std::nullopt, typecheck(unary_plus(PrimitiveValue{Bool{true}})));
  EXPECT_EQ(std::nullopt, typecheck(unary_minus(PrimitiveValue{Bool{true}})));
  EXPECT_EQ(PrimitiveType::Bool, typecheck(unary_negate(PrimitiveValue{Bool{true}})));
}
