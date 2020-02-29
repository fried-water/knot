#ifndef KNOT_H
#define KNOT_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace knot {

template <typename T>
std::vector<uint8_t> serialize(const T&);

template <typename T, typename IT>
IT serialize(const T&, IT out);

template <typename T, typename IT>
std::optional<std::decay_t<T>> deserialize(IT begin, IT end);

template <typename T, typename IT>
std::optional<std::pair<std::decay_t<T>, IT>> deserialize_partial(IT begin, IT end);

template <typename T>
std::string debug_string(const T&);

template <typename T>
std::ostream& debug_string(std::ostream&, const T&);

template <typename T>
std::size_t hash_value(const T&);

template <typename T, typename... Ts>
T from_tuple(const std::tuple<Ts...>&);

template <typename T, typename... Ts>
T from_tuple(std::tuple<Ts...>&&);

template <typename T, typename F>
void visit(const T&, F f);

template <typename T, typename F, typename S>
T accumulate(const S& t, F f, T init = {}) {
  visit(t, [&](const auto& value) {
    init = f(value, std::move(init));
    return true;
  });
  return init;
}

namespace details {

// Type Traits
template <typename, typename = void>
struct is_tieable : std::false_type {};
template <typename T>
struct is_tieable<T, std::void_t<decltype(as_tie(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_tieable_v = is_tieable<T>::value;

template <typename>
inline constexpr bool is_array_v = false;
template <typename T, std::size_t N>
inline constexpr bool is_array_v<std::array<T, N>> = true;

template <typename>
inline constexpr bool is_pair_v = false;
template <typename T1, typename T2>
inline constexpr bool is_pair_v<std::pair<T1, T2>> = true;

template <typename>
inline constexpr bool is_pointer_v = false;
template <typename T, typename Del>
inline constexpr bool is_pointer_v<std::unique_ptr<T, Del>> = true;
template <typename T>
inline constexpr bool is_pointer_v<std::shared_ptr<T>> = true;
// No support for raw pointers

template <typename>
inline constexpr bool is_tuple_v = false;
template <typename... Ts>
inline constexpr bool is_tuple_v<std::tuple<Ts...>> = true;

template <typename>
inline constexpr bool is_variant_v = false;
template <typename... Ts>
inline constexpr bool is_variant_v<std::variant<Ts...>> = true;

template <typename>
inline constexpr bool is_optional_v = false;
template <typename T>
inline constexpr bool is_optional_v<std::optional<T>> = true;

template <typename, typename = void>
struct is_range : std::false_type {};
template <typename T>
struct is_range<T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_range_v = is_range<T>::value;

// We treat tuples, pairs, and tieable structs as product types
template <typename T>
inline constexpr bool is_product_type_v = is_tieable_v<T> || is_pair_v<T> || is_tuple_v<T>;

// We treat variants, optionals, and smart ptrs as sum types
template <typename T>
inline constexpr bool is_sum_type_v = is_optional_v<T> || is_variant_v<T> || is_pointer_v<T>;

template <typename T>
inline constexpr bool is_primitive_type_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;

template <typename T>
inline constexpr bool is_knot_supported_type_v =
    is_product_type_v<T> || is_sum_type_v<T> || is_range_v<T> || is_primitive_type_v<T>;

template <typename, typename = void>
struct is_reserveable : std::false_type {};
template <typename T>
struct is_reserveable<T, std::void_t<decltype(std::declval<T>().reserve(0))>> : std::true_type {};
template <typename T>
inline constexpr bool is_reserveable_v = is_reserveable<T>::value;

template <typename T, typename T2>
using tie_enable = std::enable_if_t<is_tieable_v<T>, T2>;

template <typename>
struct tuple_from_tie;

template <typename... Ts>
struct tuple_from_tie<std::tuple<const Ts&...>> {
  using type = typename std::tuple<Ts...>;
};

template <typename T>
using tuple_from_tie_t = typename tuple_from_tie<T>::type;

// Generic product type utilities (tuple, pair, tieable structs)

template <typename T>
struct product_type_size : std::tuple_size<decltype(as_tie(std::declval<T>()))> {};

template <typename T1, typename T2>
struct product_type_size<std::pair<T1, T2>> : std::integral_constant<std::size_t, 2> {};

template <typename... Ts>
struct product_type_size<std::tuple<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename T>
inline constexpr std::size_t product_type_size_v = product_type_size<T>::value;

template <typename... Ts>
const auto& as_tuple(const std::tuple<Ts...>& tuple) {
  return tuple;
}

template <typename T1, typename T2>
auto as_tuple(const std::pair<T1, T2>& pair) {
  return std::tie(pair.first, pair.second);
}

template <typename T>
auto as_tuple(const T& tieable) {
  return as_tie(tieable);
}

// Generic sum type utilities (variant, optional, pointers)

template <typename T>
bool sum_type_index(const std::optional<T>& opt) {
  return static_cast<bool>(opt);
}

template <typename T, typename Del>
bool sum_type_index(const std::unique_ptr<T, Del>& ptr) {
  return static_cast<bool>(ptr);
}

template <typename T>
bool sum_type_index(const std::shared_ptr<T>& ptr) {
  return static_cast<bool>(ptr);
}

template <typename... Ts>
std::size_t sum_type_index(const std::variant<Ts...>& variant) {
  return variant.index();
}

template <typename T>
bool is_sum_type_null(const T& t) {
  if constexpr (is_variant_v<T>) {
    return t.valueless_by_exception();
  } else {
    return !sum_type_index(t);
  }
}

template <typename T, typename F>
void sum_type_visit(const T& t, F f) {
  if constexpr (is_variant_v<T>) {
    if (!t.valueless_by_exception()) std::visit(f, t);
  } else {
    if (t) f(*t);
  }
}

// deserialize helpers

template <typename T, typename U, typename IT, typename F>
auto with_read_result(std::optional<std::pair<U, IT>> opt, F f) {
  return opt ? std::optional{std::pair<T, IT>{f(std::move(opt->first)), opt->second}} : std::nullopt;
}

template <typename T, typename IT, std::size_t... Is>
std::optional<std::pair<T, IT>> tuple_deserialize(IT begin, IT end, std::index_sequence<Is...>) {
  static_assert(std::tuple_size_v<T> - 1 == sizeof...(Is));

  if constexpr (sizeof...(Is) == 0) {
    return with_read_result<T>(deserialize_partial<std::tuple_element_t<0, T>>(begin, end),
                               [](auto&& t) { return std::tuple(std::move(t)); });
  } else {
    using last_element = std::tuple_element_t<sizeof...(Is), T>;
    using sub_tuple = std::tuple<std::tuple_element_t<Is, T>...>;

    auto sub_tuple_opt =
        tuple_deserialize<sub_tuple>(begin, end, std::make_index_sequence<std::tuple_size_v<sub_tuple> - 1>{});
    if (!sub_tuple_opt) return std::nullopt;

    return with_read_result<T>(deserialize_partial<last_element>(sub_tuple_opt->second, end), [&](auto&& t) {
      return std::tuple_cat(std::move(sub_tuple_opt->first), std::make_tuple(std::move(t)));
    });
  }
}

template <typename Variant, typename IT, std::size_t... Is>
std::optional<std::pair<Variant, IT>> variant_deserialize(IT begin, IT end, std::size_t index,
                                                          std::index_sequence<Is...>) {
  std::array<std::optional<std::pair<Variant, IT>>, sizeof...(Is)> options{
      (index == Is ? deserialize_partial<std::variant_alternative_t<Is, Variant>>(begin, end) : std::nullopt)...};
  return index >= sizeof...(Is) ? std::nullopt : options[index];
}

// Taken from boost::hash_combine
inline std::size_t hash_combine(std::size_t seed, std::size_t hash) {
  return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

template <typename T, std::size_t... Is>
void tuple_debug_string(std::ostream& os, const T& tuple, std::index_sequence<Is...>) {
  (debug_string(os << ", ", std::get<Is + 1>(tuple)), ...);
}

template <typename T, typename Visitor, std::size_t... Is>
void visit_tuple(const T& tuple, Visitor visitor, std::index_sequence<Is...>) {
  ((visit(std::get<Is>(tuple), visitor)), ...);
};

template <typename T, typename Tuple, std::size_t... Is>
T from_tuple_helper(Tuple&& t, std::index_sequence<Is...>) {
  return T{std::get<Is>(std::forward<Tuple>(t))...};
}

}  // namespace details

template <typename T>
std::vector<uint8_t> serialize(const T& t) {
  std::vector<uint8_t> buf;
  serialize(t, std::back_inserter(buf));
  return buf;
}

template <typename Outer, typename IT>
IT serialize(const Outer& t, IT out) {
  auto serialize_primitive = [](auto value, auto it) {
    std::array<uint8_t, sizeof(value)> array{};
    std::memcpy(&array, &value, sizeof(value));
    return std::copy(array.begin(), array.end(), it);
  };

  return accumulate(t,
                    [&](const auto& value, IT out) {
                      using T = std::decay_t<decltype(value)>;
                      if constexpr (details::is_primitive_type_v<T>) {
                        return serialize_primitive(value, out);
                      } else if constexpr (details::is_sum_type_v<T>) {
                        return serialize_primitive(details::sum_type_index(value), out);
                      } else if constexpr (details::is_range_v<T>) {
                        return serialize_primitive(value.size(), out);
                      } else {
                        return out;
                      }
                    },
                    out);
}

template <typename T, typename IT>
std::optional<std::decay_t<T>> deserialize(IT begin, IT end) {
  auto opt = deserialize_partial<T>(begin, end);

  if (!opt || opt->second != end) return std::nullopt;

  return std::move(opt->first);
}

template <typename Outer, typename IT>
std::optional<std::pair<std::decay_t<Outer>, IT>> deserialize_partial(IT begin, IT end) {
  using T = std::decay_t<Outer>;

  static_assert(std::is_same_v<typename IT::value_type, uint8_t>);
  static_assert(details::is_knot_supported_type_v<T>);

  if constexpr (details::is_primitive_type_v<T>) {
    if (std::distance(begin, end) < sizeof(T)) return std::nullopt;

    std::array<uint8_t, sizeof(T)> array;
    std::copy(begin, begin + sizeof(T), array.begin());

    T t;
    std::memcpy(&t, &array, sizeof(T));

    return std::pair<T, IT>{t, begin + sizeof(T)};
  } else if constexpr (details::is_optional_v<T>) {
    if (begin == end) return std::nullopt;

    return *begin ? details::with_read_result<T>(deserialize_partial<typename T::value_type>(begin + 1, end),
                                                 [](auto&& t) { return std::optional{std::move(t)}; })
                  : std::pair<T, IT>{std::nullopt, begin + 1};
  } else if constexpr (details::is_pointer_v<T>) {
    if (begin == end) return std::nullopt;
    using element_type = typename T::element_type;
    return *begin ? details::with_read_result<T>(deserialize_partial<element_type>(begin + 1, end),
                                                 [](auto&& t) { return T{new element_type{std::move(t)}}; })
                  : std::pair<T, IT>{nullptr, begin + 1};
  } else if constexpr (details::is_variant_v<T>) {
    auto index_opt = deserialize_partial<std::size_t>(begin, end);
    if (!index_opt) return std::nullopt;

    return details::variant_deserialize<T>(index_opt->second, end, index_opt->first,
                                           std::make_index_sequence<std::variant_size_v<T>>());
  } else if constexpr (details::is_range_v<T>) {
    auto length_opt = deserialize_partial<std::size_t>(begin, end);
    if (!length_opt) return std::nullopt;

    auto [size, begin] = *length_opt;

    T range{};
    if constexpr (details::is_reserveable_v<T>) range.reserve(size);

    for (std::size_t i = 0; i < size; i++) {
      auto ele_opt = deserialize_partial<typename T::value_type>(begin, end);
      if (!ele_opt) return std::nullopt;
      if constexpr (details::is_array_v<T>) {
        range[i] = std::move(ele_opt->first);
      } else {
        range.insert(range.end(), std::move(ele_opt->first));
      }
      begin = ele_opt->second;
    }

    return std::pair<T, IT>{std::move(range), begin};
  } else if constexpr (details::is_tuple_v<T>) {
    if constexpr (std::tuple_size_v<T> == 0) {
      return std::pair<T, IT>{std::make_tuple(), begin};
    } else {
      return details::tuple_deserialize<T>(begin, end, std::make_index_sequence<std::tuple_size_v<T> - 1>{});
    }
  } else if constexpr (details::is_pair_v<T>) {
    return details::with_read_result<T>(
        deserialize_partial<std::tuple<typename T::first_type, typename T::second_type>>(begin, end), [](auto&& t) {
          return T{std::move(std::get<0>(t)), std::move(std::get<1>(t))};
        });
  } else if constexpr (details::is_tieable_v<T>) {
    return details::with_read_result<T>(
        deserialize_partial<details::tuple_from_tie_t<decltype(as_tie(std::declval<T>()))>>(begin, end),
        [](auto&& t) { return from_tuple<T>(std::move(t)); });
  } else {
    return std::nullopt;
  }
}

template <typename T>
std::string debug_string(const T& t) {
  std::ostringstream os;
  debug_string(os, t);
  return std::move(os).str();
}

template <typename T>
std::ostream& debug_string(std::ostream& os, const T& t) {
  static_assert(details::is_knot_supported_type_v<T>);

  if constexpr (std::is_arithmetic_v<T>) {
    return os << t;
  } else if constexpr (std::is_enum_v<T>) {
    return os << static_cast<std::underlying_type_t<T>>(t);
  } else if constexpr (details::is_sum_type_v<T>) {
    if (details::is_sum_type_null(t)) {
      return os << "None";
    } else {
      os << "<";
      details::sum_type_visit(t, [&os](const auto& inner) { debug_string(os, inner); });
      return os << ">";
    }
  } else if constexpr (std::is_same_v<T, std::string>) {
    // Special case string
    return os << "\"" << t << "\"";
  } else if constexpr (details::is_range_v<T>) {
    os << "[";
    auto it = t.begin();
    if (it != t.end()) {
      debug_string(os, *it);
      std::for_each(++it, t.end(), [&os](const auto& ele) { debug_string(os << ", ", ele); });
    }
    return os << "]";
  } else if constexpr (details::is_product_type_v<T>) {
    const auto& tuple = details::as_tuple(t);
    os << "(";
    if constexpr (details::product_type_size_v<T>> 0) {
      debug_string(os, std::get<0>(tuple));
      details::tuple_debug_string(os, tuple, std::make_index_sequence<details::product_type_size_v<T> - 1>());
    }
    return os << ")";
  } else {
    return os;
  }
}

template <typename Outer>
std::size_t hash_value(const Outer& t) {
  return accumulate<std::size_t>(t, [&](const auto& value, const std::size_t hash) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (details::is_primitive_type_v<T>) {
      return details::hash_combine(hash, std::hash<T>{}(value));
    } else if constexpr (details::is_sum_type_v<T>) {
      return details::hash_combine(hash, details::sum_type_index(value));
    } else {
      return hash;
    }
  });
}

template <typename T, typename Visitor>
void visit(const T& t, Visitor visitor) {
  static_assert(details::is_knot_supported_type_v<T>);

  // visit the value
  if (!visitor(t)) return;

  // Call visit recursively depending on type
  if constexpr (details::is_product_type_v<T>) {
    details::visit_tuple(details::as_tuple(t), visitor, std::make_index_sequence<details::product_type_size_v<T>>());
  } else if constexpr (details::is_sum_type_v<T>) {
    details::sum_type_visit(t, [&](const auto& val) { return visit(val, visitor); });
  } else if constexpr (details::is_range_v<T>) {
    for (const auto& val : t) visit(val, visitor);
  }
}

template <typename Result, typename... Ts>
Result from_tuple(const std::tuple<Ts...>& t) {
  return details::from_tuple_helper<Result>(t, std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename Result, typename... Ts>
Result from_tuple(std::tuple<Ts...>&& t) {
  return details::from_tuple_helper<Result>(std::move(t), std::make_index_sequence<sizeof...(Ts)>{});
}

// std::hash<T> replacement for tieable types
struct Hash {
  template <typename T>
  std::size_t operator()(const T& t) const {
    return knot::hash_value(t);
  }
};

//
struct Compareable {
  template <typename T>
  friend details::tie_enable<T, bool> operator==(const T& lhs, const T& rhs) {
    return as_tie(lhs) == as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator!=(const T& lhs, const T& rhs) {
    return as_tie(lhs) != as_tie(rhs);
  }
};

struct Ordered : Compareable {
  template <typename T>
  friend details::tie_enable<T, bool> operator<(const T& lhs, const T& rhs) {
    return as_tie(lhs) < as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator<=(const T& lhs, const T& rhs) {
    return as_tie(lhs) <= as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator>(const T& lhs, const T& rhs) {
    return as_tie(lhs) > as_tie(rhs);
  }

  template <typename T>
  friend details::tie_enable<T, bool> operator>=(const T& lhs, const T& rhs) {
    return as_tie(lhs) >= as_tie(rhs);
  }
};

}  // namespace knot

#endif
