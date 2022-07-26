#pragma once

#include "knot/auto_as_tie.h"
#include "knot/type_traits.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <variant>

namespace knot {

enum class TypeCategory : uint8_t { Unknown, Primative, Range, Product, Sum, Maybe };

constexpr inline auto is_tieable = is_valid([](auto&& t) -> decltype(as_tie(t)) {});

template <typename T>
constexpr auto tie_type(Type<T>) {
  constexpr auto tie = decay(Type<decltype(as_tie(std::declval<T>()))>{});
  if constexpr (is_tuple_like(tie)) {
    return as_tuple(map(as_typelist(tie), [](auto t) { return decay(t); }));
  } else {
    return tie;
  }
}

template <typename T>
constexpr bool is_variant(Type<T>) {
  return false;
}

template <typename... Ts>
constexpr bool is_variant(Type<std::variant<Ts...>>) {
  return true;
}

template <typename T>
constexpr bool is_pointer(Type<T> t) {
  return is_raw_pointer(t);
}

template <typename T, typename D>
constexpr bool is_pointer(Type<std::unique_ptr<T, D>>) {
  return true;
}

template <typename T>
constexpr bool is_pointer(Type<std::shared_ptr<T>>) {
  return true;
}

template <typename T>
constexpr bool is_optional(Type<T>) {
  return false;
}

template <typename T>
constexpr bool is_optional(Type<std::optional<T>>) {
  return true;
}

template <typename T>
constexpr bool is_range(Type<T> t) {
  return is_valid([](auto&& t) -> decltype(t.begin()) {})(t) && is_valid([](auto&& t) -> decltype(t.end()) {})(t);
}

template <typename T>
constexpr TypeCategory category(Type<T> t) {
  if constexpr (is_tieable(t)) {
    return category(tie_type(t));
  } else if constexpr (is_enum(t) || is_arithmetic(t)) {
    return TypeCategory::Primative;
  } else if constexpr (is_range(t)) {
    return TypeCategory::Range;
  } else if constexpr (is_tuple_like(t)) {
    return TypeCategory::Product;
  } else if constexpr (is_variant(t)) {
    return TypeCategory::Sum;
  } else if constexpr (is_optional(t) || is_pointer(t)) {
    return TypeCategory::Maybe;
  } else {
    return TypeCategory::Unknown;
  }
}

template <typename T>
constexpr bool is_supported(Type<T> t) {
  return category(t) != TypeCategory::Unknown;
}

}  // namespace knot
