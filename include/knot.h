#pragma once

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

// std as_tie() overload
template <typename First, typename Second>
auto as_tie(const std::pair<First, Second>& pair) {
  return std::tie(pair.first, pair.second);
}

template <typename First, typename Second>
auto as_tie(std::pair<First, Second>&& pair) {
  return std::forward_as_tuple(std::move(pair.first), std::move(pair.second));
}

constexpr inline auto is_tieable = is_valid([](auto&& t) -> decltype(as_tie(t)) {});

template <typename T>
constexpr auto tie_type(Type<T>) {
  constexpr auto tie = decay(Type<decltype(as_tie(std::declval<T>()))>{});
  if constexpr (is_tuple(tie)) {
    return as_tuple(map(as_typelist(tie), [](auto t) { return decay(t); }));
  } else {
    return tie;
  }
}

template<size_t N>
struct Names {
  constexpr Names() = default;

  constexpr explicit Names(std::string_view name) : name(name) {}

  constexpr explicit Names(std::string_view name, const std::string_view (&members_)[N])
    : name(name)
  {
    std::copy(std::begin(members_), std::end(members_), std::begin(members));
  }
  constexpr explicit Names(const std::string_view (&members_)[N]) {
    std::copy(std::begin(members_), std::end(members_), std::begin(members));
  }

  constexpr static size_t member_count() { return N; }

  std::optional<std::string_view> name;
  std::array<std::string_view, N> members;
};

Names(std::string_view) -> Names<0>;

constexpr inline auto has_names = is_valid([](auto&& t) -> decltype(names(decay(Type<decltype(t)>{}))) {});
constexpr inline auto is_reserveable = is_valid([](auto&& t) -> decltype(t.reserve(0)) {});

enum class TypeCategory { Unknown, Primative, Range, Product, Sum, Maybe };

template <typename T>
constexpr TypeCategory category(Type<T> t) {
  if constexpr (is_tieable(t)) {
    return category(tie_type(t));
  } else if constexpr (is_enum(t) || is_arithmetic(t)) {
    return TypeCategory::Primative;
  } else if constexpr (is_range(t)) {
    return TypeCategory::Range;
  } else if constexpr (is_tuple(t)) {
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

namespace details {

template <typename T>
using decayed_tie_t = typename decltype(tie_type(Type<T>{}))::type;

// Generic maybe type utilities (optional, pointers)

template <typename Result>
std::enable_if_t<is_optional(Type<Result>{}), Result> from_optional(Result&& op) {
  return std::move(op);
}

template <typename Result, typename T>
std::enable_if_t<is_pointer(Type<Result>{}), Result> from_optional(std::optional<T>&& op) {
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
    return make_monad(opt ? std::optional(f(std::move(opt->first), std::move(opt->second))) : std::nullopt);
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
    return std::pair(std::tuple(), begin);
  } else {
    using first_element = std::remove_const_t<std::tuple_element_t<0, T>>;
    using tuple_rest = typename tuple_rest<T>::type;

    return make_monad(deserialize_partial<first_element>(begin, end)).and_then([end](first_element&& first, IT begin) {
      return make_monad(tuple_deserialize<tuple_rest>(begin, end))
          .map([&first](tuple_rest&& rest, IT begin) {
            return std::pair(T{std::tuple_cat(std::make_tuple(std::move(first)), std::move(rest))}, begin);
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
        .map([](auto&& ele, IT begin) { return std::pair(Variant{std::move(ele)}, begin); })
        .get();
  }...};
  return index >= sizeof...(Is) ? std::nullopt : options[index](begin, end);
}

template <typename T, typename Visitor, std::size_t... Is>
void visit_tuple(const T& tuple, Visitor visitor, std::index_sequence<Is...>) {
  ((visitor(std::get<Is>(tuple))), ...);
}

template <typename Result, typename Tuple, typename F, typename T, std::size_t... Is>
Result map_tuple(T&& t, F f, std::index_sequence<Is...>) {
  static_assert(std::tuple_size_v<std::decay_t<T>> == sizeof...(Is));
  return Result{map<std::tuple_element_t<Is, Tuple>>(std::get<Is>(std::forward<T>(t)), f)...};
}

template<typename T, size_t N, typename F, size_t... Is>
void named_visit(const T& tuple, const Names<N>& names, F f, std::index_sequence<Is...>) {
  (f(names.members[Is], std::get<Is>(tuple)), ...);
}

template<typename T, size_t N, typename F>
void named_visit(const T& tuple, const Names<N>& names, F f) {
  static_assert(N == std::tuple_size_v<T>);
  named_visit(tuple, names, f, std::make_index_sequence<N>());
}

}  // namespace details

template <typename T>
std::vector<std::byte> serialize(const T& t) {
  std::vector<std::byte> buf;
  serialize(t, std::back_inserter(buf));
  return buf;
}

template <typename T, typename IT>
IT serialize(const T& t, IT it) {
  constexpr Type<T> type = {};

  static_assert(is_supported(type), "Unsupported type in serialize");

  if constexpr (is_tieable(type)) {
    return serialize(as_tie(t), it);
  } else if constexpr (category(type) == TypeCategory::Primative) {
    std::array<std::byte, sizeof(T)> array;
    std::memcpy(array.data(), &t, sizeof(T));
    return std::transform(array.begin(), array.end(), it, [](std::byte b) {
      if constexpr (is_valid([](auto&& it) -> decltype(*it = std::byte{}) {})(Type<IT>{})) {
        return b;
      } else {
        return static_cast<uint8_t>(b);
      }
    });
  } else if constexpr (category(type) == TypeCategory::Sum) {
    return accumulate<IT>(t, [&](IT it, const auto& ele) { return serialize(ele, it); },
      serialize(t.index(), it));
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    return accumulate<IT>(t, [&](IT it, const auto& ele) { return serialize(ele, it); },
      serialize(static_cast<bool>(t), it));
  } else if constexpr (category(type) == TypeCategory::Range) {
    return accumulate<IT>(t, [&](IT it, const auto& ele) { return serialize(ele, it); },
      serialize(t.size(), it));
  } else if constexpr (category(type) == TypeCategory::Product) {
    return accumulate<IT>(t, [&](IT it, const auto& ele) { return serialize(ele, it); }, it);
  } else {
    return it;
  }
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

  constexpr Type<T> type = {};
  constexpr auto it_type = decay(Type<decltype(*begin)>{});

  static_assert(is_supported(type) && !is_ref(type) && !is_raw_pointer(type));
  static_assert(it_type == Type<uint8_t>{} || it_type == Type<int8_t>{} || it_type == Type<std::byte>{});

  if constexpr (is_tieable(type)) {
    return details::make_monad(deserialize_partial<details::decayed_tie_t<T>>(begin, end))
        .map([](auto tied_type, IT begin) { return std::pair(map<T>(std::move(tied_type)), begin); });
  } else if constexpr (category(type) == TypeCategory::Primative) {
    if (std::distance(begin, end) < sizeof(T)) return std::nullopt;

    std::array<std::byte, sizeof(T)> array;
    std::transform(begin, begin + sizeof(T), array.begin(), [](auto b) { return std::byte{static_cast<uint8_t>(b)}; });

    T t;
    std::memcpy(&t, array.data(), sizeof(T));

    return std::pair<T, IT>{t, begin + sizeof(T)};
  } else if constexpr (category(type) == TypeCategory::Sum) {
    return details::make_monad(deserialize_partial<std::size_t>(begin, end))
        .and_then([end](std::size_t index, IT begin) {
          return details::variant_deserialize<T>(begin, end, index, std::make_index_sequence<std::variant_size_v<T>>());
        });
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    using optional_t = std::optional<std::decay_t<decltype(*std::declval<T>())>>;
    return details::make_monad(deserialize_partial<bool>(begin, end))
        .and_then([end](bool has_value, IT begin) -> std::optional<std::pair<optional_t, IT>> {
          if (has_value) {
            return details::make_monad(deserialize_partial<typename optional_t::value_type>(begin, end))
                .map([](auto&& inner, IT begin) { return std::pair(std::make_optional(std::move(inner)), begin); });
          } else {
            return std::pair(std::nullopt, begin);
          }
        })
        .map([](optional_t&& optional, IT begin) {
          return std::pair(details::from_optional<T>(std::move(optional)), begin);
        });
  } else if constexpr (category(type) == TypeCategory::Range) {
    return details::make_monad(deserialize_partial<std::size_t>(begin, end))
        .map([&](std::size_t size, IT begin) -> std::optional<std::pair<T, IT>> {
          T range{};
          if constexpr (is_reserveable(type)) range.reserve(size);

          for (std::size_t i = 0; i < size; i++) {
            auto ele_opt = deserialize_partial<typename T::value_type>(begin, end);
            if (!ele_opt) return std::nullopt;
            if constexpr (is_array(type)) {
              range[i] = std::move(ele_opt->first);
            } else {
              range.insert(range.end(), std::move(ele_opt->first));
            }
            begin = ele_opt->second;
          }

          return std::pair<T, IT>{std::move(range), begin};
        });
  } else if constexpr (category(type) == TypeCategory::Product) {
    return details::tuple_deserialize<T>(begin, end);
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
  struct DebugOverloads {
    void operator()(std::ostream& os, const std::string& str) const { os << str; }
    void operator()(std::ostream& os, const char* str) const { os << str; }
    void operator()(std::ostream& os, std::string_view str) const { os << str; }
  };

  constexpr Type<T> type = {};

  static_assert(is_supported(type) || std::is_invocable_v<DebugOverloads, std::ostream&, T>);

  if constexpr (std::is_invocable_v<DebugOverloads, std::ostream&, T>) {
    DebugOverloads{}(os, t);
  } else if constexpr (type == Type<bool>{}) {
    os << (t ? "true" : "false"); // special case bool due to implicit conversions
  } else if constexpr (is_tieable(type) && !has_names(type)) {
    return debug(os, as_tie(t));
  } else if constexpr (is_tieable(type)) {
    const auto t_names = names(type);
    if (t_names.name) {
      os << *t_names.name;
    }

    os << '(';
    if constexpr (is_tuple(tie_type(type))) {
      static_assert(t_names.member_count() == size(tie_type(type)));
      int i = 0;
      details::named_visit(as_tie(t), t_names, [&](const auto& name, const auto& inner) {
        debug(i++ == 0 ? os << name << ": " : os << ", " << name << ": ", inner);
      });
    } else {
      debug(os, as_tie(t));
    }

    os << ')';
  } else if constexpr (is_arithmetic(type)) {
    os << t;
  } else if constexpr (is_enum(type)) {
    os << static_cast<std::underlying_type_t<T>>(t);
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    static_cast<bool>(t) ? debug(os, *t) : os << "none";
  } else if constexpr (category(type) == TypeCategory::Sum) {
    visit(t, [&](const auto& inner) { debug(os, inner); });
  } else if constexpr (category(type) == TypeCategory::Range || category(type) == TypeCategory::Product) {
    if constexpr (category(type) == TypeCategory::Product) {
      os << '(';
    } else {
      const auto size = std::distance(std::begin(t), std::end(t));
      os << '[' << size << "; ";
    }

    int i = 0;
    visit(t, [&](const auto& inner) {
      debug(i++ == 0 ? os : os << ", ", inner);
    });

    os << (category(type) == TypeCategory::Product ? ')' : ']');
  }

  return os;
}

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

    return std::accumulate(bytes.begin(), bytes.end(), static_cast<std::size_t>(0), [&](std::size_t acc, std::byte b) {
      return hash_combine(acc, static_cast<std::size_t>(b));
    });
  } else if constexpr (is_supported(type)) {
    std::size_t initial_value = 0;
    if constexpr (category(type) == TypeCategory::Sum) {
      initial_value = t.index();
    } else if constexpr (category(type) == TypeCategory::Maybe) {
      initial_value = static_cast<std::size_t>(static_cast<bool>(t));
    }

    return accumulate<std::size_t>(t,
      [&](std::size_t acc, const auto& ele) { return hash_combine(acc, hash_value(ele)); },
      initial_value);
  } else {
    return 0;
  }
}

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
    return accumulate<std::size_t>(t, [](std::size_t acc, const auto& ele) { return acc + area(ele); });
  } else if constexpr (std::is_trivially_destructible_v<T>) {
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
  constexpr Type<T> type = {};

  const auto try_visit = [&](const auto& val) {
    if constexpr (std::is_invocable_v<Visitor, decltype(val)>) {
      visitor(val);
    }
  };

  if constexpr (is_tieable(type)) {
    knot::visit(as_tie(t), visitor);
  } else if constexpr (is_tuple(type)) {
    details::visit_tuple(t, try_visit, std::make_index_sequence<std::tuple_size_v<T>>());
  } else if constexpr (is_variant(type)) {
    knot::visit_variant(t, try_visit);
  } else if constexpr (is_pointer(type) || is_optional(type)) {
    if (static_cast<bool>(t)) try_visit(*t);
  } else if constexpr (category(type) == TypeCategory::Range) {
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
  constexpr auto type = decay(Type<T>{});
  constexpr Type<Result> result_type = {};

  // Type category needs to align
  static_assert(std::is_invocable_r_v<Result, F, std::decay_t<T>> || category(result_type) == category(type));

  // Not mapping to raw pointers
  static_assert(!std::is_pointer_v<Result>);

  if constexpr (std::is_invocable_r_v<Result, F, std::decay_t<T>>) {
    return f(std::forward<T>(t));
  } else if constexpr (type == result_type) {
    return std::forward<T>(t);
  } else if constexpr (is_tieable(type)) {
    return map<Result>(as_tie(std::forward<T>(t)), f);
  } else if constexpr (is_tieable(result_type)) {
    using ResultTiedType = details::decayed_tie_t<Result>;
    if constexpr (is_tuple(Type<ResultTiedType>{})) {
      return details::map_tuple<Result, ResultTiedType>(std::forward<T>(t), f,
        std::make_index_sequence<std::tuple_size_v<ResultTiedType>>());
    } else {
      return Result{map<ResultTiedType>(std::forward<T>(t), f)};
    }
  } else if constexpr (category(result_type) == TypeCategory::Primative) {
    return static_cast<Result>(t);
  } else if constexpr (category(result_type) == TypeCategory::Product) {
    return details::map_tuple<Result, Result>(std::forward<T>(t), f,
      std::make_index_sequence<std::tuple_size_v<Result>>());
  } else if constexpr (category(result_type) == TypeCategory::Sum) {
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
  } else if constexpr (is_pointer(result_type)) {
    using ElementT = typename Result::element_type;
    return t ? Result{new ElementT{map<ElementT>(*std::forward<T>(t), f)}} : nullptr;
  } else if constexpr (is_optional(result_type)) {
    return t ? Result{map<typename Result::value_type>(*std::forward<T>(t), f)} : std::nullopt;
  } else if constexpr (category(result_type) == TypeCategory::Range) {
    Result range{};
    if constexpr (is_reserveable(result_type)) range.reserve(std::distance(std::begin(t), std::end(t)));

    using DstType = typename Result::value_type;

    int i = 0;
    // TODO bound check result arrays
    if constexpr (is_ref(Type<T>{})) {
      for (const auto& val : t) {
        if constexpr (is_array(result_type)) {
          range[i++] = map<DstType>(val, f);
        } else {
          range.insert(range.end(), map<DstType>(val, f));
        }
      }
    } else {
      for (auto&& val : t) {
        if constexpr (is_array(result_type)) {
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
