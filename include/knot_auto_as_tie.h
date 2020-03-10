#ifndef KNOT_AUTO_AS_TIE
#define KNOT_AUTO_AS_TIE

#include "knot_type_traits.h"

namespace knot {
namespace details {

// This auto generates as_tie() for aggregate structs with no base classes
template <typename T>
auto as_tie(const T& t, std::enable_if_t<std::is_aggregate_v<T> && !is_array_v<T> && !std::is_polymorphic_v<T> &&
                                             // Techinically it can't have any base classes
                                             // no way to check that generically
                                             arity<T>() <= 16,
                                         int> = 0) {
  constexpr std::size_t my_arity = arity<T>();

  if constexpr (my_arity == 0) {
    return std::tie();
  } else if constexpr (my_arity == 1) {
    const auto& [a] = t;
    return std::tie(a);
  } else if constexpr (my_arity == 2) {
    const auto& [a, b] = t;
    return std::tie(a, b);
  } else if constexpr (my_arity == 3) {
    const auto& [a, b, c] = t;
    return std::tie(a, b, c);
  } else if constexpr (my_arity == 4) {
    const auto& [a, b, c, d] = t;
    return std::tie(a, b, c, d);
  } else if constexpr (my_arity == 5) {
    const auto& [a, b, c, d, e] = t;
    return std::tie(a, b, c, d, e);
  } else if constexpr (my_arity == 6) {
    const auto& [a, b, c, d, e, f] = t;
    return std::tie(a, b, c, d, e, f);
  } else if constexpr (my_arity == 7) {
    const auto& [a, b, c, d, e, f, g] = t;
    return std::tie(a, b, c, d, e, f, g);
  } else if constexpr (my_arity == 8) {
    const auto& [a, b, c, d, e, f, g, h] = t;
    return std::tie(a, b, c, d, e, f, g, h);
  } else if constexpr (my_arity == 9) {
    const auto& [a, b, c, d, e, f, g, h, i] = t;
    return std::tie(a, b, c, d, e, f, g, h, i);
  } else if constexpr (my_arity == 10) {
    const auto& [a, b, c, d, e, f, g, h, i, j] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j);
  } else if constexpr (my_arity == 11) {
    const auto& [a, b, c, d, e, f, g, h, i, j, k] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j, k);
  } else if constexpr (my_arity == 12) {
    const auto& [a, b, c, d, e, f, g, h, i, j, k, l] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j, k, l);
  } else if constexpr (my_arity == 13) {
    const auto& [a, b, c, d, e, f, g, h, i, j, k, l, m] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m);
  } else if constexpr (my_arity == 14) {
    const auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, n] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
  } else if constexpr (my_arity == 15) {
    const auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
  } else if constexpr (my_arity == 16) {
    const auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = t;
    return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
  }
}

}  // namespace details
}  // namespace knot

#endif