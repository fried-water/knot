#pragma once

#define KNOT_COMPAREABLE(U)                                                               \
  template <typename KNOT_T>                                                              \
  std::enable_if_t<std::is_same_v<U, KNOT_T>, bool> operator==(const KNOT_T& rhs) const { \
    using knot::as_tie;                                                                   \
    return as_tie(*this) == as_tie(rhs);                                                  \
  }                                                                                       \
                                                                                          \
  template <typename KNOT_T>                                                              \
  std::enable_if_t<std::is_same_v<U, KNOT_T>, bool> operator!=(const KNOT_T& rhs) const { \
    using knot::as_tie;                                                                   \
    return as_tie(*this) != as_tie(rhs);                                                  \
  }

#define KNOT_ORDERED(U)                                                                   \
  KNOT_COMPAREABLE(U)                                                                     \
                                                                                          \
  template <typename KNOT_T>                                                              \
  std::enable_if_t<std::is_same_v<U, KNOT_T>, bool> operator<(const KNOT_T& rhs) const {  \
    using knot::as_tie;                                                                   \
    return as_tie(*this) < as_tie(rhs);                                                   \
  }                                                                                       \
                                                                                          \
  template <typename KNOT_T>                                                              \
  std::enable_if_t<std::is_same_v<U, KNOT_T>, bool> operator<=(const KNOT_T& rhs) const { \
    using knot::as_tie;                                                                   \
    return as_tie(*this) <= as_tie(rhs);                                                  \
  }                                                                                       \
                                                                                          \
  template <typename KNOT_T>                                                              \
  std::enable_if_t<std::is_same_v<U, KNOT_T>, bool> operator>(const KNOT_T& rhs) const {  \
    using knot::as_tie;                                                                   \
    return as_tie(*this) > as_tie(rhs);                                                   \
  }                                                                                       \
                                                                                          \
  template <typename KNOT_T>                                                              \
  std::enable_if_t<std::is_same_v<U, KNOT_T>, bool> operator>=(const KNOT_T& rhs) const { \
    using knot::as_tie;                                                                   \
    return as_tie(*this) >= as_tie(rhs);                                                  \
  }
