#pragma once

#include "knot/traversals.h"
#include "knot/type_category.h"

namespace knot {

template <typename T>
std::size_t area(const T&);

template <typename T>
std::size_t area(const std::vector<T>&);

template <typename T>
std::size_t area(const std::unique_ptr<T>&);

template <typename T>
std::size_t area(const T& t) {
  constexpr Type<T> type = {};

  static_assert(is_supported(type) || std::is_trivially_destructible_v<T>, "Unsupported type in area()");

  if constexpr (is_tieable(type)) {
    return area(as_tie(t));
  } else if constexpr (is_supported(type)) {
    return accumulate(t, std::size_t{0}, [](std::size_t acc, const auto& ele) { return acc + area(ele); });
  } else if constexpr (std::is_trivially_destructible_v<T>) {
    return 0;
  }
}

template <typename T>
std::size_t area(const std::vector<T>& v) {
  return accumulate(v, v.capacity() * sizeof(T), [](std::size_t acc, const auto& t) { return acc + area(t); });
}

template <typename T>
std::size_t area(const std::unique_ptr<T>& ptr) {
  return ptr ? (sizeof(T) + area(*ptr)) : 0;
}

}  // namespace knot
