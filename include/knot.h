#ifndef KNOT_H
#define KNOT_H

#include "knot_auto_as_tie.h"
#include "knot_ops.h"
#include "knot_type_traits.h"
#include "knot_visit_variant.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <numeric>
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
std::vector<std::byte> serialize(const T&);

template <typename T, typename IT>
IT serialize(const T&, IT out);

// In addition to as_tie(), deserialize requires that structs be constructible
// from the types returned in as_tie().
// Furthermore raw pointers and references aren't supported.
template <typename T, typename IT>
std::optional<T> deserialize(IT begin, IT end);

template <typename T, typename IT>
std::optional<std::pair<T, IT>> deserialize_partial(IT begin, IT end);

template <typename T>
std::string debug(const T&);

template <typename T>
std::ostream& debug(std::ostream&, const T&);

template <typename T>
std::size_t hash_value(const T&);

template <typename T, typename F>
void visit(const T&, F);

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc = {});

template <typename T, typename F>
void preorder(const T&, F);

template <typename Result, typename T, typename F>
Result preorder_accumulate(const T& t, F f, Result acc = {});

template <typename T, typename F>
void postorder(const T&, F);

template <typename T>
std::size_t size(const T& t) {
  return accumulate<std::size_t>(t, [](std::size_t acc, const auto&) { return acc + 1; });
}

template <typename T>
std::size_t area(const T&);

template <typename Result, typename T, typename F = std::tuple<>>
Result map(T&&, F = {});

namespace details {

// std as_tie() overload
template <typename First, typename Second>
auto as_tie(const std::pair<First, Second>& pair) {
  return std::tie(pair.first, pair.second);
}

template <typename First, typename Second>
auto as_tie(std::pair<First, Second>&& pair) {
  return std::forward_as_tuple(std::move(pair.first), std::move(pair.second));
}

template <typename, typename = void>
struct is_tieable : std::false_type {};
template <typename T>
struct is_tieable<T, std::void_t<decltype(as_tie(std::declval<T>()))>> : std::true_type {};
template <typename T>
inline constexpr bool is_tieable_v = is_tieable<T>::value;

// We treat tuples, pairs, and tieable structs as product types
template <typename T>
inline constexpr bool is_product_type_v = is_tieable_v<T> || is_tuple_v<T>;

// We treat optionals, and ptrs as maybe types
template <typename T>
inline constexpr bool is_maybe_type_v = is_optional_v<T> || is_pointer_v<T>;

template <typename T>
inline constexpr bool is_primitive_type_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;

template <typename T>
inline constexpr bool is_knot_supported_type_v =
    is_product_type_v<T> || is_variant_v<T> || is_maybe_type_v<T> || is_range_v<T> || is_primitive_type_v<T>;

// Generic product type utilities (tuple, pair, tieable structs)

template <typename, typename = void>
struct decayed_tuple;

template <typename... Ts>
struct decayed_tuple<std::tuple<Ts...>> {
  using type = std::tuple<std::decay_t<Ts>...>;
};

template <typename T>
struct decayed_tuple<T, std::enable_if_t<is_tieable_v<T>>> : decayed_tuple<decltype(as_tie(std::declval<T>()))> {};

template <typename T>
using decayed_tuple_t = typename decayed_tuple<T>::type;

template <typename... Ts>
const auto& as_tuple(const std::tuple<Ts...>& tuple) {
  return tuple;
}

template <typename... Ts>
std::tuple<Ts...>&& as_tuple(std::tuple<Ts...>&& tuple) {
  return std::move(tuple);
}

template <typename T>
auto as_tuple(T&& tieable) {
  return as_tie(std::forward<T>(tieable));
}

// Generic maybe type utilities (optional, pointers)

template <typename Result>
std::enable_if_t<is_optional_v<Result>, Result> from_optional(Result&& op) {
  return std::move(op);
}

template <typename Result, typename T>
std::enable_if_t<is_pointer_v<Result>, Result> from_optional(std::optional<T>&& op) {
  return Result{static_cast<bool>(op) ? new T{std::move(*op)} : nullptr};
}

// Monadic optional wrapper for help with deserialize

template <typename T>
struct Monad;

template <typename T>
auto make_monad(T&& t) {
  return Monad<std::decay_t<T>>{std::forward<T>(t)};
}

template <typename First, typename Second>
struct Monad<std::optional<std::pair<First, Second>>> {
  std::optional<std::pair<First, Second>> opt;

  template <typename F>
  auto and_then(F f) && {
    return make_monad(opt ? f(std::move(opt->first), std::move(opt->second)) : std::nullopt);
  }

  template <typename F>
  auto map(F f) && {
    return make_monad(opt ? std::make_optional(f(std::move(opt->first), std::move(opt->second))) : std::nullopt);
  }

  operator std::optional<std::pair<First, Second>>() && { return std::move(opt); }
  std::optional<std::pair<First, Second>> get() && { return std::move(opt); }
};

// deserialize helpers

template <typename>
struct tuple_rest;

template <typename T, typename... Rest>
struct tuple_rest<std::tuple<T, Rest...>> {
  using type = std::tuple<Rest...>;
};

template <typename T, typename IT>
std::optional<std::pair<T, IT>> tuple_deserialize(IT begin, IT end) {
  if constexpr (std::tuple_size_v<T> == 0) {
    static_cast<void>(end);  // To suppress warnings about not touching end on this branch
    return std::make_pair(std::make_tuple(), begin);
  } else {
    using first_element = std::remove_const_t<std::tuple_element_t<0, T>>;
    using tuple_rest = typename tuple_rest<T>::type;

    return make_monad(deserialize_partial<first_element>(begin, end)).and_then([end](first_element&& first, IT begin) {
      return make_monad(tuple_deserialize<tuple_rest>(begin, end))
          .map([&first](tuple_rest&& rest, IT begin) {
            return std::make_pair(T{std::tuple_cat(std::make_tuple(std::move(first)), std::move(rest))}, begin);
          })
          .get();
    });
  }
}

template <typename Variant, typename IT, std::size_t... Is>
std::optional<std::pair<Variant, IT>> variant_deserialize(IT begin, IT end, std::size_t index,
                                                          std::index_sequence<Is...>) {
  static constexpr auto options = std::array{+[](IT begin, IT end) {
    return make_monad(deserialize_partial<std::variant_alternative_t<Is, Variant>>(begin, end))
        .map([](auto&& ele, IT begin) { return std::make_pair(Variant{std::move(ele)}, begin); })
        .get();
  }...};
  return index >= sizeof...(Is) ? std::nullopt : options[index](begin, end);
}

template <typename T, typename Visitor, std::size_t... Is>
void visit_tuple(const T& tuple, Visitor visitor, std::index_sequence<Is...>) {
  ((visitor(std::get<Is>(tuple))), ...);
}

template <typename Result, typename F, typename T, std::size_t... Is>
Result map_tuple(T&& t, F f, std::index_sequence<Is...>) {
  return Result{
      map<std::tuple_element_t<Is, decayed_tuple_t<Result>>>(std::get<Is>(as_tuple(std::forward<T>(t))), f)...};
}

template <typename Result, typename T, std::size_t... Is>
Result make_from_tuple(T&& t, std::index_sequence<Is...>) {
  return Result{std::get<Is>(std::forward<T>(t))...};
}

struct DebugOverloads {
  void operator()(std::ostream& os, const std::string& str) const { os << str; }
  void operator()(std::ostream& os, const char* str) const { os << str; }
  void operator()(std::ostream& os, std::string_view str) const { os << str; }
};

}  // namespace details

template <typename T>
std::vector<std::byte> serialize(const T& t) {
  std::vector<std::byte> buf;
  serialize(t, std::back_inserter(buf));
  return buf;
}

template <typename Outer, typename IT>
IT serialize(const Outer& t, IT out) {
  auto serialize_primitive = [](auto value, auto it) {
    std::array<std::byte, sizeof(value)> array{};
    std::memcpy(&array, &value, sizeof(value));
    return std::transform(array.begin(), array.end(), it,
                          [](std::byte b) { return static_cast<details::output_it_value_t<IT>>(b); });
  };

  return preorder_accumulate(
      t,
      [&](IT out, const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (details::is_primitive_type_v<T>) {
          return serialize_primitive(value, out);
        } else if constexpr (details::is_variant_v<T>) {
          return serialize_primitive(value.index(), out);
        } else if constexpr (details::is_maybe_type_v<T>) {
          return serialize_primitive(static_cast<bool>(value), out);
        } else if constexpr (details::is_range_v<T>) {
          return serialize_primitive(std::distance(value.begin(), value.end()), out);
        } else {
          return out;
        }
      },
      out);
}

template <typename T, typename IT>
std::optional<T> deserialize(IT begin, IT end) {
  auto opt = deserialize_partial<T>(begin, end);

  if (!opt || opt->second != end) return std::nullopt;

  return std::move(opt->first);
}

template <typename Outer, typename IT>
std::optional<std::pair<Outer, IT>> deserialize_partial(IT begin, IT end) {
  using T = std::remove_const_t<Outer>;
  using it_value_t = typename IT::value_type;

  static_assert(!std::is_reference_v<Outer>);
  static_assert(!std::is_pointer_v<T>);
  static_assert(std::is_same_v<it_value_t, uint8_t> || std::is_same_v<it_value_t, int8_t> ||
                std::is_same_v<it_value_t, std::byte>);
  static_assert(details::is_knot_supported_type_v<T>);

  if constexpr (details::is_primitive_type_v<T>) {
    if (std::distance(begin, end) < sizeof(T)) return std::nullopt;

    std::array<std::byte, sizeof(T)> array;
    std::transform(begin, begin + sizeof(T), array.begin(), [](auto b) { return std::byte{static_cast<uint8_t>(b)}; });

    T t;
    std::memcpy(&t, &array, sizeof(T));

    return std::pair<T, IT>{t, begin + sizeof(T)};
  } else if constexpr (details::is_variant_v<T>) {
    return details::make_monad(deserialize_partial<std::size_t>(begin, end))
        .and_then([end](std::size_t index, IT begin) {
          return details::variant_deserialize<T>(begin, end, index, std::make_index_sequence<std::variant_size_v<T>>());
        });
  } else if constexpr (details::is_maybe_type_v<T>) {
    using optional_t = std::optional<std::decay_t<decltype(*std::declval<T>())>>;
    return details::make_monad(deserialize_partial<bool>(begin, end))
        .and_then([end](bool has_value, IT begin) -> std::optional<std::pair<optional_t, IT>> {
          if (has_value) {
            return details::make_monad(deserialize_partial<typename optional_t::value_type>(begin, end))
                .map(
                    [](auto&& inner, IT begin) { return std::make_pair(std::make_optional(std::move(inner)), begin); });
          } else {
            return std::make_pair(std::nullopt, begin);
          }
        })
        .map([](optional_t&& optional, IT begin) {
          return std::make_pair(details::from_optional<T>(std::move(optional)), begin);
        });
  } else if constexpr (details::is_range_v<T>) {
    return details::make_monad(deserialize_partial<std::size_t>(begin, end))
        .and_then([end](std::size_t size, IT begin) -> std::optional<std::pair<T, IT>> {
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
        });
  } else if constexpr (details::is_product_type_v<T>) {
    return details::make_monad(details::tuple_deserialize<details::decayed_tuple_t<T>>(begin, end))
        .map([](auto&& tuple, IT begin) {
          return std::make_pair(
              details::make_from_tuple<T>(std::move(tuple),
                                          std::make_index_sequence<std::tuple_size_v<details::decayed_tuple_t<T>>>()),
              begin);
        });
  } else {
    return std::nullopt;
  }
}

template <typename T>
std::string debug(const T& t) {
  std::ostringstream os;
  debug(os, t);
  return std::move(os).str();
}

template <typename T>
std::ostream& debug(std::ostream& os, const T& t) {
  static_assert(details::is_knot_supported_type_v<T> || std::is_invocable_v<details::DebugOverloads, std::ostream& ,T>);

  if constexpr(std::is_invocable_v<details::DebugOverloads, std::ostream&, T>) {
    details:: DebugOverloads{}(os, t);
  } else if constexpr(std::is_same_v<T, bool>) {
    os << (t ? "true" : "false"); // special case bool due to implicit conversions
  } else if constexpr (std::is_arithmetic_v<T>) {
    os << t;
  } else if constexpr (std::is_enum_v<T>) {
    os << static_cast<std::underlying_type_t<T>>(t);
  } else if constexpr (details::is_maybe_type_v<T>) {
    static_cast<bool>(t) ? debug(os, *t) : os << "none";
  } else if constexpr (details::is_variant_v<T> || details::is_range_v<T> || details::is_product_type_v<T>) {
    if constexpr (details::is_variant_v<T>)
      os << "<";
    else if constexpr (details::is_product_type_v<T>)
      os << "(";
    else
      os << "[" << std::distance(t.begin(), t.end()) << "; ";

    int i = 0;
    visit(t, [&](const auto& inner) {
      debug(i++ == 0 ? os : os << ", ", inner);
    });

    if constexpr (details::is_variant_v<T>)
      os << ">";
    else if constexpr (details::is_product_type_v<T>)
      os << ")";
    else
      os << "]";
  }

  return os;
}

template <typename Outer>
std::size_t hash_value(const Outer& t) {
  return preorder_accumulate<std::size_t>(t, [&](const std::size_t hash, const auto& value) {
    // Taken from boost::hash_combine
    const auto hash_combine = [](std::size_t seed, std::size_t hash) {
      return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    };

    using T = std::decay_t<decltype(value)>;
    if constexpr (details::is_primitive_type_v<T> ||
                  (!details::is_knot_supported_type_v<T> && details::is_std_hashable_v<T>)) {
      return hash_combine(hash, std::hash<T>{}(value));
    } else if constexpr (details::is_variant_v<T>) {
      return hash_combine(hash, value.index());
    } else if constexpr (details::is_maybe_type_v<T>) {
      return hash_combine(hash, static_cast<bool>(value));
    } else {
      return hash;
    }
  });
}

template <typename T>
std::size_t area(const std::vector<T>&);

template <typename T>
std::size_t area(const std::unique_ptr<T>&);

template <typename T>
std::size_t area(const T& t) {
  const auto area_visitor = [](const auto& t) { return area(t); };

  if constexpr (std::is_trivially_destructible_v<T>) {
    return 0;
  } else if constexpr (details::is_array_v<T>) {
    return std::accumulate(t.begin(), t.end(), static_cast<std::size_t>(0),
                           [](std::size_t acc, const auto& t) { return acc + area(t); });
  } else if constexpr (details::is_product_type_v<T>) {
    return accumulate<std::size_t>(t, [](std::size_t acc, const auto& ele) { return acc + area(ele); });
  } else if constexpr (details::is_variant_v<T>) {
    return visit_variant(t, area_visitor);
  } else if constexpr (details::is_optional_v<T>) {
    return t ? area(*t) : 0;
  } else {
    static_assert(!std::is_same_v<T, T>, "Unsupported type in area()");
    return 0;
  }
}

template <typename T>
std::size_t area(const std::vector<T>& v) {
  return std::accumulate(v.begin(), v.end(), v.capacity() * sizeof(T),
                         [](std::size_t acc, const auto& t) { return acc + area(t); });
}

template <typename T>
std::size_t area(const std::unique_ptr<T>& ptr) {
  return ptr ? (sizeof(T) + area(*ptr)) : 0;
}

template <typename T, typename Visitor>
void visit(const T& t, Visitor visitor) {
  const auto try_visit = [&](const auto& val) {
    if constexpr (std::is_invocable_v<Visitor, decltype(val)>) {
      visitor(val);
    }
  };

  if constexpr (details::is_tieable_v<T>) {
    using knot::details::as_tie;
    details::visit_tuple(as_tie(t), try_visit, std::make_index_sequence<std::tuple_size_v<decltype(as_tie(t))>>());
  } else if constexpr (details::is_tuple_v<T>) {
    details::visit_tuple(t, try_visit, std::make_index_sequence<std::tuple_size_v<T>>());
  } else if constexpr (details::is_variant_v<T>) {
    knot::visit_variant(t, try_visit);
  } else if constexpr (details::is_maybe_type_v<T>) {
    if (static_cast<bool>(t)) try_visit(*t);
  } else if constexpr (details::is_range_v<T>) {
    for (const auto& val : t) try_visit(val);
  }
}

template <typename Result, typename T, typename F>
Result accumulate(const T& t, F f, Result acc) {
  visit(t, [&](const auto& value) {
    if constexpr (std::is_invocable_r_v<Result, F, Result, decltype(value)>) {
      acc = f(std::move(acc), value);
    }
  });
  return acc;
}

template <typename T, typename Visitor>
void preorder(const T& t, Visitor visitor) {
  // visit the value, if the function returns a bool, stop recursing on false
  if constexpr (std::is_invocable_r_v<bool, Visitor, T>) {
    if (!visitor(t)) return;
  } else if constexpr (std::is_invocable_v<Visitor, T>) {
    visitor(t);
  }

  visit(t, [&](const auto& val) { preorder(val, visitor); });
}

template <typename Result, typename T, typename F>
Result preorder_accumulate(const T& t, F f, Result acc) {
  preorder(t, [&](const auto& value) {
    if constexpr (std::is_invocable_r_v<Result, F, Result, decltype(value)>) {
      acc = f(std::move(acc), value);
    }
  });
  return acc;
}

template <typename T, typename Visitor>
void postorder(const T& t, Visitor visitor) {
  visit(t, [&](const auto& val) { postorder(val, visitor); });

  if constexpr (std::is_invocable_v<Visitor, T>) {
    visitor(t);
  }
}

template <typename Result, typename T, typename F>
Result map(T&& t, F f) {
  using DecayedT = std::decay_t<T>;

  // Type category needs to align
  static_assert(std::is_invocable_r_v<Result, F, DecayedT> ||
                (details::is_primitive_type_v<Result> == details::is_primitive_type_v<DecayedT> &&
                 details::is_product_type_v<Result> == details::is_product_type_v<DecayedT> &&
                 details::is_variant_v<Result> == details::is_variant_v<DecayedT> &&
                 details::is_maybe_type_v<Result> == details::is_maybe_type_v<DecayedT> &&
                 details::is_range_v<Result> == details::is_range_v<DecayedT>));

  // Not mapping to raw pointers
  static_assert(!std::is_pointer_v<Result>);

  if constexpr (std::is_invocable_r_v<Result, F, DecayedT>) {
    return f(std::forward<T>(t));
  } else if constexpr (std::is_same_v<Result, DecayedT>) {
    return std::forward<T>(t);
  } else if constexpr (details::is_primitive_type_v<Result>) {
    return static_cast<Result>(t);
  } else if constexpr (details::is_product_type_v<Result>) {
    constexpr std::size_t SrcSize = std::tuple_size_v<details::decayed_tuple_t<Result>>;
    constexpr std::size_t DstSize = std::tuple_size_v<details::decayed_tuple_t<DecayedT>>;

    static_assert(SrcSize == DstSize);

    return details::map_tuple<Result>(std::forward<T>(t), f, std::make_index_sequence<SrcSize>());
  } else if constexpr (details::is_variant_v<Result>) {
    // can only map from equivalent variants or if the Result variant is a superset of types
    return std::visit(
        [&](auto&& val) -> Result {
          if constexpr (std::is_invocable_v<F, std::decay_t<decltype(val)>>) {
            return f(std::forward<decltype(val)>(val));
          } else {
            return std::forward<decltype(val)>(val);
          }
        },
        std::forward<T>(t));
  } else if constexpr (details::is_pointer_v<Result>) {
    using ElementT = typename Result::element_type;
    return t ? Result{new ElementT{map<ElementT>(*std::forward<T>(t), f)}} : nullptr;
  } else if constexpr (details::is_optional_v<Result>) {
    return t ? Result{map<typename Result::value_type>(*std::forward<T>(t), f)} : std::nullopt;
  } else if constexpr (details::is_range_v<Result>) {
    Result range{};
    if constexpr (details::is_reserveable_v<Result>) range.reserve(std::distance(std::begin(t), std::end(t)));

    using DstType = typename Result::value_type;

    int i = 0;
    // TODO bound check result arrays
    if constexpr (std::is_reference_v<T>) {
      for (const auto& val : t) {
        if constexpr (details::is_array_v<Result>) {
          range[i++] = map<DstType>(val, f);
        } else {
          range.insert(range.end(), map<DstType>(val, f));
        }
      }
    } else {
      for (auto&& val : t) {
        if constexpr (details::is_array_v<Result>) {
          range[i++] = map<DstType>(std::move(val), f);
        } else {
          range.insert(range.end(), map<DstType>(std::move(val), f));
        }
      }
    }

    return range;
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

#endif
