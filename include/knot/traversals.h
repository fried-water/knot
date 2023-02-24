#pragma once

#include "knot/type_category.h"
#include "knot/type_traits.h"

namespace knot {

template <typename T, typename F>
void visit(T&&, F);

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc = {});

template <typename T, typename F>
void preorder(const T&, F);

template <typename Result, typename T, typename F>
Result preorder_accumulate(const T& t, F f, Result acc = {});

template <typename T, typename F>
void postorder(const T&, F);

template <typename T>
std::size_t size(const T& t) {
  return accumulate<std::size_t>(t, [](std::size_t acc, const auto&) { return acc + 1; });
}

template <typename T>
std::size_t preorder_size(const T& t) {
  return preorder_accumulate<std::size_t>(t, [](std::size_t acc, const auto&) { return acc + 1; });
}

namespace details {

template <typename T, typename Visitor, std::size_t... Is>
void visit_tuple_like(T&& tuple, Visitor visitor, std::index_sequence<Is...>) {
  ((visitor(std::get<Is>(std::forward<T>(tuple)))), ...);
}

}  // namespace details

template <typename T, typename Visitor>
void visit(T&& t, Visitor visitor) {
  constexpr auto type = decay(Type<T>{});

  const auto try_visit = [&](auto&& val) {
    if constexpr (is_invocable(Type<Visitor>{}, TypeList<decltype(val)>{})) {
      visitor(std::forward<decltype(val)>(val));
    }
  };

  if constexpr (is_tieable(type)) {
    knot::visit(as_tie(std::forward<T>(t)), visitor);
  } else if constexpr (category(type) == TypeCategory::Product) {
    details::visit_tuple_like(std::forward<T>(t), try_visit, idx_seq(type));
  } else if constexpr (category(type) == TypeCategory::Sum) {
    std::visit(try_visit, std::forward<T>(t));
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    if (static_cast<bool>(t)) try_visit(*std::forward<T>(t));
  } else if constexpr (category(type) == TypeCategory::Range) {
    for (auto&& val : t) {
      if constexpr(is_ref(Type<T>{})) {
        try_visit(val);
      } else {
        try_visit(std::move(val));
      }
    }
  }
}

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc) {
  visit(t, [&](const auto& value) {
    if constexpr (Type<Result>{} == invoke_result(Type<F>{}, TypeList<Result, decltype(value)>{})) {
      acc = f(std::move(acc), value);
    }
  });
  return acc;
}

template <typename T, typename Visitor>
void preorder(const T& t, Visitor visitor) {
  // visit the value, if the function returns a bool, stop recursing on false
  if constexpr (Type<bool>{} == invoke_result(Type<Visitor>{}, TypeList<T>{})) {
    if (!visitor(t)) return;
  } else if constexpr (is_invocable(Type<Visitor>{}, TypeList<T>{})) {
    visitor(t);
  }

  visit(t, [&](const auto& val) { preorder(val, visitor); });
}

template <typename Result, typename T, typename F>
Result preorder_accumulate(const T& t, F f, Result acc) {
  preorder(t, [&](const auto& value) {
    if constexpr (Type<Result>{} == invoke_result(Type<F>{}, TypeList<Result, decltype(value)>{})) {
      acc = f(std::move(acc), value);
    }
  });
  return acc;
}

template <typename T, typename Visitor>
void postorder(const T& t, Visitor visitor) {
  visit(t, [&](const auto& val) { postorder(val, visitor); });

  if constexpr (is_invocable(Type<Visitor>{}, TypeList<T>{})) {
    visitor(t);
  }
}

}  // namespace knot
