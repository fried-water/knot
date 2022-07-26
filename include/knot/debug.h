#pragma once

#include "knot/auto_as_tie.h"
#include "knot/type_category.h"
#include "knot/type_traits.h"

#include <array>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace knot {

template <typename T>
std::string debug(const T&);

template <typename T>
std::ostream& debug(std::ostream&, const T&);

template <size_t N>
struct Names {
  constexpr Names() = default;

  constexpr explicit Names(std::string_view name) : name(name) {}

  constexpr explicit Names(std::string_view name, const std::string_view (&members_)[N]) : name(name) {
    std::copy(std::begin(members_), std::end(members_), std::begin(members));
  }
  constexpr explicit Names(const std::string_view (&members_)[N]) {
    std::copy(std::begin(members_), std::end(members_), std::begin(members));
  }

  constexpr static size_t member_count() { return N; }

  std::optional<std::string_view> name;
  std::array<std::string_view, N> members;
};

Names(std::string_view)->Names<0>;

constexpr inline auto has_names = is_valid([](auto&& t) -> decltype(names(decay(Type<decltype(t)>{}))) {});

template <typename T, size_t N, typename F, size_t... Is>
void named_visit(const T& tuple, const Names<N>& names, F f, std::index_sequence<Is...>) {
  (f(names.members[Is], std::get<Is>(tuple)), ...);
}

template <typename... Ts, size_t N, typename F>
void named_visit(const std::tuple<Ts...>& tuple, const Names<N>& names, F f) {
  static_assert(N == sizeof...(Ts));
  named_visit(tuple, names, f, idx_seq(Type<std::tuple<Ts...>>{}));
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

  constexpr bool use_overloads = is_invocable(Type<DebugOverloads>{}, TypeList<std::ostream&, T>{});

  static_assert(is_supported(type) || use_overloads);

  if constexpr (use_overloads) {
    DebugOverloads{}(os, t);
  } else if constexpr (type == Type<bool>{}) {
    os << (t ? "true" : "false");  // special case bool due to implicit conversions
  } else if constexpr (is_tieable(type) && !has_names(type)) {
    return debug(os, as_tie(t));
  } else if constexpr (is_tieable(type)) {
    const auto t_names = names(type);
    if (t_names.name) {
      os << *t_names.name;
    }

    os << '(';
    if constexpr (is_tuple_like(tie_type(type))) {
      static_assert(t_names.member_count() == size(as_typelist(tie_type(type))));
      int i = 0;
      named_visit(as_tie(t), t_names, [&](const auto& name, const auto& inner) {
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
    visit(t, [&](const auto& inner) { debug(i++ == 0 ? os : os << ", ", inner); });

    os << (category(type) == TypeCategory::Product ? ')' : ']');
  }

  return os;
}

}  // namespace knot
