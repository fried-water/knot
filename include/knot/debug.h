#pragma once

#include "knot/auto_as_tie.h"
#include "knot/type_category.h"
#include "knot/type_traits.h"

#include "knot/traversals.h"

#include <array>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace knot {

struct MultiLine {
  int collapse_threshold = 10;
  int tab_size = 2;
  int indentation = 0;
};

template <typename T>
std::string debug(const T&, std::optional<MultiLine> = {});

template <typename T>
std::ostream& debug(std::ostream&, const T&, std::optional<MultiLine> = {});

template <size_t N>
struct Names {
  constexpr Names() = default;

  constexpr explicit Names(std::string_view name) : name(name) {}

  constexpr explicit Names(std::string_view name, const std::string_view (&members_)[N]) : name(name) {
    for(size_t i = 0; i < N; i++) {
      members[i] = members_[i];
    }
  }
  constexpr explicit Names(const std::string_view (&members_)[N]) {
    for(size_t i = 0; i < N; i++) {
      members[i] = members_[i];
    }
  }

  constexpr static size_t member_count() { return N; }

  std::optional<std::string_view> name;
  std::array<std::string_view, N> members = {};
};

Names(std::string_view) -> Names<0>;

constexpr inline auto has_names = is_valid([](auto&& t) -> decltype(names(decay(Type<decltype(t)>{}))) {});

template <typename T>
std::string debug(const T& t, std::optional<MultiLine> multi) {
  std::ostringstream os;
  debug(os, t, multi);
  return std::move(os).str();
}

namespace {

template<typename T>
std::ostream& debug_list(
  std::ostream& os,
  const T& t,
  std::optional<MultiLine> multi,
  bool include_size = false,
  const std::string_view* names = nullptr)
{
  constexpr Type<T> type = {};

  const auto size = knot::size(t);

  const char delimeter = multi ? '\n' : ' ';
  const int indentation = multi ? multi->indentation : 0;
  const int tab_size = multi ? multi->tab_size : 0;

  const auto next_multi = multi
    ? std::optional(MultiLine{multi->collapse_threshold, tab_size, indentation + tab_size})
    : std::nullopt;

  const auto indent = [&](int amount) {
    for(int i = 0; i < amount; i++) os << ' ';
  };

  os << (category(type) == TypeCategory::Product ? '(' : '[');

  if(include_size) {
    os << size << ';';
    if(size > 0 || multi) os << delimeter;
  } else if(multi) {
    os << '\n';
  }

  int i = 0;

  visit(t, [&](const auto& inner) {
    indent(indentation + tab_size);
    debug(names == nullptr ? os : os << names[i] << ": ", inner, next_multi);
    if(++i < size) os << ',';
    if(i < size || multi) os << delimeter;
  });

  indent(indentation);
  return os << (category(type) == TypeCategory::Product ? ')' : ']');
}

}

template <typename T>
std::ostream& debug(std::ostream& os, const T& t, std::optional<MultiLine> multi) {
  struct DebugOverloads {
    void operator()(std::ostream& os, const std::string& str) const { os << str; }
    void operator()(std::ostream& os, const char* str) const { os << str; }
    void operator()(std::ostream& os, std::string_view str) const { os << str; }
  };

  constexpr Type<T> type = {};

  constexpr bool use_overloads = is_invocable(Type<DebugOverloads>{}, TypeList<std::ostream&, T>{});

  static_assert(is_supported(type) || use_overloads);

  if(multi) {
    int count = 0;
    preorder(t, [&](const auto&) { return ++count <= multi->collapse_threshold; });
    multi = count <= multi->collapse_threshold ? std::nullopt : multi;
  }

  if constexpr (use_overloads) {
    DebugOverloads{}(os, t);
  } else if constexpr (type == Type<bool>{}) {
    os << (t ? "true" : "false");  // special case bool due to implicit conversions
  } else if constexpr (is_tieable(type) && !has_names(type)) {
    return debug(os, as_tie(t), multi);
  } else if constexpr (is_tieable(type)) {
    constexpr auto t_names = names(type);
    if (t_names.name) {
      os << *t_names.name;
    }

    if constexpr (is_tuple_like(tie_type(type))) {
      static_assert(t_names.member_count() == 0 || t_names.member_count() == size(as_typelist(tie_type(type))));

      if constexpr(t_names.member_count() == 0) {
        debug_list(os, as_tie(t), multi);
      } else {
        debug_list(os, as_tie(t), multi, false, t_names.members.data());
      }
    } else {
      debug(os << '(', as_tie(t), multi) << ')';
    }
  } else if constexpr (is_arithmetic(type)) {
    os << t;
  } else if constexpr (is_enum(type)) {
    os << static_cast<std::underlying_type_t<T>>(t);
  } else if constexpr (category(type) == TypeCategory::Maybe) {
    static_cast<bool>(t) ? debug(os, *t, multi) : os << "none";
  } else if constexpr (category(type) == TypeCategory::Sum) {
    visit(t, [&](const auto& inner) { debug(os, inner, multi); });
  } else if constexpr (category(type) == TypeCategory::Range || category(type) == TypeCategory::Product) {
    debug_list(os, t, multi, category(type) == TypeCategory::Range);
  }

  return os;
}

}  // namespace knot
