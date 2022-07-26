#pragma once

#include "knot/traversals.h"
#include "knot/type_category.h"
#include "knot/type_traits.h"

#include <array>
#include <cstddef>
#include <cstring>

namespace knot {

template <typename T>
std::size_t hash_value(const T& t) {
  // Taken from boost::hash_combine
  const auto hash_combine = [](std::size_t seed, std::size_t hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
  };

  constexpr Type<T> type = {};

  static_assert(is_supported(type) || std::has_unique_object_representations_v<T>, "Unsupported type in hash");

  if constexpr (is_tieable(type)) {
    return hash_value(as_tie(t));
  } else if constexpr (is_valid([](auto&& t) -> decltype(std::hash<T>{}(t)) {})(type)) {
    return std::hash<T>{}(t);
  } else if constexpr (std::has_unique_object_representations_v<T>) {
    std::array<std::byte, sizeof(T)> bytes;
    std::memcpy(bytes.data(), &t, sizeof(T));

    return accumulate<std::size_t>(bytes, hash_combine);
  } else if constexpr (is_supported(type)) {
    std::size_t initial_value = 0;
    if constexpr (category(type) == TypeCategory::Sum) {
      initial_value = t.index();
    } else if constexpr (category(type) == TypeCategory::Maybe) {
      initial_value = static_cast<std::size_t>(static_cast<bool>(t));
    }

    return accumulate<std::size_t>(
        t, [&](std::size_t acc, const auto& ele) { return hash_combine(acc, hash_value(ele)); }, initial_value);
  } else {
    return 0;
  }
}

// std::hash<T> replacement for tieable types
struct Hash {
  template <typename T>
  std::size_t operator()(const T& t) const noexcept {
    return knot::hash_value(t);
  }
};

}  // namespace knot
