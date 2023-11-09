#pragma once

#include "knot/traversals.h"
#include "knot/type_category.h"
#include "knot/type_traits.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <functional>

namespace knot {

template <typename T>
std::size_t hash_value(const T& t) {
  // Taken from boost::hash_combine
  const auto hash_combine = [](std::size_t seed, std::size_t hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  };

  constexpr Type<T> type = {};

  static_assert(is_supported(type), "Unsupported type in hash");

  if constexpr (is_tieable(type)) {
    return hash_value(as_tie(t));
  } else if constexpr (category(type) == TypeCategory::Primitive || is_raw_pointer(type)) {
    return std::hash<T>{}(t);
  } else if constexpr (is_supported(type)) {
    std::size_t initial_value = 0;
    if constexpr (category(type) == TypeCategory::Sum) {
      initial_value = t.index();
    } else if constexpr (category(type) == TypeCategory::Maybe) {
      initial_value = static_cast<std::size_t>(static_cast<bool>(t));
    }

    return accumulate(t, initial_value,
                      [&](std::size_t acc, const auto& ele) { return hash_combine(acc, hash_value(ele)); });
  } else {
    return 0;
  }
}

// std::hash<T> replacement for tieable types
struct Hash {
  template <typename T>
  std::size_t operator()(const T& t) const noexcept {
    return hash_value(t);
  }
};

}  // namespace knot
