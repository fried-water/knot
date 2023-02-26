#pragma once

#include "knot/type_category.h"
#include "knot/type_traits.h"

namespace knot {

template <typename T, typename F>
void visit(T&&, F);

template <typename T, typename Result, typename F>
Result accumulate(T&& t, Result acc, F f);

template <typename T, typename F>
void preorder(T&&, F);

template <typename T, typename Result, typename F>
Result preorder_accumulate(T&& t, Result acc, F f);

template <typename T, typename F>
void postorder(T&&, F);

template <typename T>
std::size_t size(const T& t) {
  return accumulate(t, std::size_t{0}, [](std::size_t acc, const auto&) { return acc + 1; });
}

template <typename T>
std::size_t preorder_size(const T& t) {
  return preorder_accumulate(t, std::size_t{0}, [](std::size_t acc, const auto&) { return acc + 1; });
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
      if constexpr (is_ref(Type<T>{})) {
        try_visit(val);
      } else {
        try_visit(std::move(val));
      }
    }
  }
}

template <typename T, typename Result, typename F>
Result accumulate(T&& t, Result acc, F f) {
  visit(std::forward<T>(t), [&](auto&& v) {
    if constexpr (Type<Result>{} == invoke_result(Type<F>{}, TypeList<Result, decltype(v)>{})) {
      acc = f(std::move(acc), std::forward<decltype(v)>(v));
    }
  });
  return acc;
}

template <typename T, typename Visitor>
void preorder(T&& t, Visitor visitor) {
  // visit the value, if the function returns a bool, stop recursing on false
  if constexpr (Type<bool>{} == invoke_result(Type<Visitor>{}, TypeList<T>{})) {
    if (!visitor(std::forward<T>(t))) return;
  } else if constexpr (is_invocable(Type<Visitor>{}, TypeList<T>{})) {
    visitor(std::forward<T>(t));
  }

  visit(std::forward<T>(t), [&](auto&& v) { preorder(std::forward<decltype(v)>(v), visitor); });
}

template <typename T, typename Result, typename F>
Result preorder_accumulate(T&& t, Result acc, F f) {
  preorder(std::forward<T>(t), [&](auto&& v) {
    if constexpr (Type<Result>{} == invoke_result(Type<F>{}, TypeList<Result, decltype(v)>{})) {
      acc = f(std::move(acc), std::forward<decltype(v)>(v));
    }
  });
  return acc;
}

template <typename T, typename Visitor>
void postorder(T&& t, Visitor visitor) {
  visit(std::forward<T>(t), [&](auto&& v) { postorder(std::forward<decltype(v)>(v), visitor); });

  if constexpr (is_invocable(Type<Visitor>{}, TypeList<T>{})) {
    visitor(std::forward<T>(t));
  }
}

}  // namespace knot
