#pragma once

#include "knot/type_traits.h"

#include <utility>

namespace not_knot {

constexpr inline auto is_user_tieable = knot::is_valid([](auto&& t) -> decltype(as_tie(t)) {});

}

namespace knot {
namespace details {

struct filler {
  template <typename T>
  operator T() const;
};

template <typename T, typename Seq = std::index_sequence<>, typename = void>
struct aggregate_arity : Seq {};

template <typename T, std::size_t... Is>
struct aggregate_arity<T, std::index_sequence<Is...>, std::void_t<decltype(T{filler{}, (Is, filler{})...})>>
    : aggregate_arity<T, std::index_sequence<Is..., sizeof...(Is)>> {};

template <typename T>
constexpr size_t arity(Type<T> t) {
  if constexpr (is_aggregate(t)) {
    return aggregate_arity<T>::size();
  } else {
    return -1;
  }
}

template <typename T>
struct any_base {
  operator T() = delete;

  template <typename U, typename = std::enable_if_t<std::is_base_of_v<U, T>>>
  operator U();
};

template <typename, typename = void>
struct has_any_base : std::false_type {};
template <typename T>
struct has_any_base<T, std::void_t<decltype(T{any_base<T>{}})>> : std::true_type {};

template <typename T, typename U>
constexpr decltype(auto) aliasing_forward(U&& obj) noexcept {
  if constexpr (std::is_lvalue_reference_v<T>) {
    return obj;
  } else {
    return std::move(obj);
  }
}

template <typename T>
constexpr bool is_auto_tieable(Type<T> t) {
  if constexpr(is_aggregate(t)) {
      return arity(t) <= 16
        && !not_knot::is_user_tieable(t)
        && !is_array(t)
        && !has_any_base<std::decay_t<T>>::value;
  } else {
    return false;
  }
}

}  // namespace details

// This auto generates as_tie() for aggregate structs with no base classes
template <typename T>
auto as_tie(T&& t, std::enable_if_t<details::is_auto_tieable(decay(Type<T>{})), int> = 0) {
  constexpr std::size_t my_arity = details::arity(decay(Type<T>{}));

  if constexpr (my_arity == 0) {
    return std::forward_as_tuple();
  } else if constexpr (my_arity == 1) {
    auto&& [a] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a));
  } else if constexpr (my_arity == 2) {
    auto&& [a, b] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b));
  } else if constexpr (my_arity == 3) {
    auto&& [a, b, c] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c));
  } else if constexpr (my_arity == 4) {
    auto&& [a, b, c, d] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c), details::aliasing_forward<T>(d));
  } else if constexpr (my_arity == 5) {
    auto&& [a, b, c, d, e] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c), details::aliasing_forward<T>(d),
                                 details::aliasing_forward<T>(e));
  } else if constexpr (my_arity == 6) {
    auto&& [a, b, c, d, e, f] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c), details::aliasing_forward<T>(d),
                                 details::aliasing_forward<T>(e), details::aliasing_forward<T>(f));
  } else if constexpr (my_arity == 7) {
    auto&& [a, b, c, d, e, f, g] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c), details::aliasing_forward<T>(d),
                                 details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
                                 details::aliasing_forward<T>(g));
  } else if constexpr (my_arity == 8) {
    auto&& [a, b, c, d, e, f, g, h] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c), details::aliasing_forward<T>(d),
                                 details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
                                 details::aliasing_forward<T>(g), details::aliasing_forward<T>(h));
  } else if constexpr (my_arity == 9) {
    auto&& [a, b, c, d, e, f, g, h, i] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i));
  } else if constexpr (my_arity == 10) {
    auto&& [a, b, c, d, e, f, g, h, i, j] = std::forward<T>(t);
    return std::forward_as_tuple(details::aliasing_forward<T>(a), details::aliasing_forward<T>(b),
                                 details::aliasing_forward<T>(c), details::aliasing_forward<T>(d),
                                 details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
                                 details::aliasing_forward<T>(g), details::aliasing_forward<T>(h),
                                 details::aliasing_forward<T>(i), details::aliasing_forward<T>(j));
  } else if constexpr (my_arity == 11) {
    auto&& [a, b, c, d, e, f, g, h, i, j, k] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i),
        details::aliasing_forward<T>(j), details::aliasing_forward<T>(k));
  } else if constexpr (my_arity == 12) {
    auto&& [a, b, c, d, e, f, g, h, i, j, k, l] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i),
        details::aliasing_forward<T>(j), details::aliasing_forward<T>(k), details::aliasing_forward<T>(l));
  } else if constexpr (my_arity == 13) {
    auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i),
        details::aliasing_forward<T>(j), details::aliasing_forward<T>(k), details::aliasing_forward<T>(l),
        details::aliasing_forward<T>(m));
  } else if constexpr (my_arity == 14) {
    auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m, n] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i),
        details::aliasing_forward<T>(j), details::aliasing_forward<T>(k), details::aliasing_forward<T>(l),
        details::aliasing_forward<T>(m), details::aliasing_forward<T>(n));
  } else if constexpr (my_arity == 15) {
    auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i),
        details::aliasing_forward<T>(j), details::aliasing_forward<T>(k), details::aliasing_forward<T>(l),
        details::aliasing_forward<T>(m), details::aliasing_forward<T>(n), details::aliasing_forward<T>(o));
  } else if constexpr (my_arity == 16) {
    auto&& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = std::forward<T>(t);
    return std::forward_as_tuple(
        details::aliasing_forward<T>(a), details::aliasing_forward<T>(b), details::aliasing_forward<T>(c),
        details::aliasing_forward<T>(d), details::aliasing_forward<T>(e), details::aliasing_forward<T>(f),
        details::aliasing_forward<T>(g), details::aliasing_forward<T>(h), details::aliasing_forward<T>(i),
        details::aliasing_forward<T>(j), details::aliasing_forward<T>(k), details::aliasing_forward<T>(l),
        details::aliasing_forward<T>(m), details::aliasing_forward<T>(n), details::aliasing_forward<T>(o),
        details::aliasing_forward<T>(p));
  }
}

}  // namespace knot
