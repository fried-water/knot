#ifndef KNOT_OPS
#define KNOT_OPS

#define KNOT_COMPAREABLE(U)                                                     \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator==(const T& rhs) const { \
    using knot::details::as_tie;                                                \
    return as_tie(*this) == as_tie(rhs);                                        \
  }                                                                             \
                                                                                \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator!=(const T& rhs) const { \
    using knot::details::as_tie;                                                \
    return as_tie(*this) != as_tie(rhs);                                        \
  }

#define KNOT_ORDERED(U)                                                         \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator==(const T& rhs) const { \
    using knot::details::as_tie;                                                \
    return as_tie(*this) == as_tie(rhs);                                        \
  }                                                                             \
                                                                                \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator!=(const T& rhs) const { \
    using knot::details::as_tie;                                                \
    return as_tie(*this) != as_tie(rhs);                                        \
  }                                                                             \
                                                                                \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator<(const T& rhs) const {  \
    using knot::details::as_tie;                                                \
    return as_tie(*this) < as_tie(rhs);                                         \
  }                                                                             \
                                                                                \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator<=(const T& rhs) const { \
    using knot::details::as_tie;                                                \
    return as_tie(*this) <= as_tie(rhs);                                        \
  }                                                                             \
                                                                                \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator>(const T& rhs) const {  \
    using knot::details::as_tie;                                                \
    return as_tie(*this) > as_tie(rhs);                                         \
  }                                                                             \
                                                                                \
  template <typename T>                                                         \
  std::enable_if_t<std::is_same_v<U, T>, bool> operator>=(const T& rhs) const { \
    using knot::details::as_tie;                                                \
    return as_tie(*this) >= as_tie(rhs);                                        \
  }

#endif