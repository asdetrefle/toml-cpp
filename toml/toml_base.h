#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#define TOML_STANDARD toml_1_0_0_rc1
#define TOML_INLINE_NAMESPACE_NAME lts_2020_04_09

#define TOML_NAMESPACE_BEGIN                    \
    inline namespace TOML_INLINE_NAMESPACE_NAME \
    {
#define TOML_NAMESPACE_END }

namespace toml
{
TOML_NAMESPACE_BEGIN

template <typename T, typename... U>
using is_one_of = std::disjunction<std::is_same<T, U>...>;

template <typename T, typename... U>
inline constexpr bool is_one_of_v = is_one_of<T, U...>::value;

/// Type traits class to convert C++ types to enum member : internal storage types
template <class T>
struct base_type_traits
{
    using type = void;
    static constexpr auto value = base_type::None;
};

template <>
struct base_type_traits<std::string>
{
    using type = std::string;
    static constexpr auto value = base_type::String;
};

template <>
struct base_type_traits<int64_t>
{
    using type = int64_t;
    static constexpr auto value = base_type::Integer;
};

template <>
struct base_type_traits<double>
{
    using type = double;
    static constexpr auto value = base_type::Float;
};

template <>
struct base_type_traits<bool>
{
    using type = bool;
    static constexpr auto value = base_type::Boolean;
};

template <>
struct base_type_traits<offset_date_time>
{
    using type = offset_date_time;
    static constexpr auto value = base_type::OffsetDateTime;
};

template <>
struct base_type_traits<local_date_time>
{
    using type = local_date_time;
    static constexpr auto value = base_type::LocalDateTime;
};

template <>
struct base_type_traits<local_date>
{
    using type = local_date;
    static constexpr auto value = base_type::LocalDate;
};

template <>
struct base_type_traits<local_time>
{
    using type = local_time;
    static constexpr auto value = base_type::LocalTime;
};

template <>
struct base_type_traits<table>
{
    using type = table;
    static constexpr auto value = base_type::Table;
};

template <>
struct base_type_traits<array>
{
    using type = array;
    static constexpr auto value = base_type::Array;
};

/// Type traits class to convert C++ types to enum member
template <class T, class Enable = void>
struct value_type_traits
{
    using type = typename base_type_traits<T>::type;
    static constexpr auto value = base_type_traits<T>::value;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<is_one_of_v<T,
                                                                  int32_t,
                                                                  int16_t,
                                                                  int8_t,
                                                                  uint64_t,
                                                                  uint32_t,
                                                                  uint16_t,
                                                                  uint8_t>>>
{
    using type = T;
    static constexpr auto value = base_type::Integer;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
{
    using type = T;
    static constexpr auto value = base_type::Float;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<is_one_of_v<std::decay_t<T>,
                                                                  std::string_view,
                                                                  const char *>>>
{
    using type = std::string_view;
    static constexpr auto value = base_type::String;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<std::is_same_v<T, table>>>
{
    using type = std::shared_ptr<table>;
    static constexpr auto value = base_type::Table;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<std::is_same_v<T, array>>>
{
    using type = std::shared_ptr<array>;
    static constexpr auto value = base_type::Array;
};

#if __cplusplus > 201703
using remove_cvref_t = std::remove_cvref_t<T>;
#else
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

template <typename T>
inline constexpr bool is_value_or_node = (base_type_traits<T>::value != base_type::None);

template <typename T>
inline constexpr bool is_value = (base_type_traits<T>::value != base_type::None) &&
                                 static_cast<uint8_t>(base_type_traits<T>::value) < 9;

template <typename T>
inline constexpr bool is_value_promotable = (value_type_traits<T>::value != base_type::None) &&
                                            static_cast<uint8_t>(value_type_traits<T>::value) < 9;

inline constexpr bool is_char = std::is_same_v<const char (&)[1], const char *>;

template <class U>
inline std::shared_ptr<value<typename value_type_traits<U>::type>> make_value(U &&val);
inline std::shared_ptr<array> make_array();
inline std::shared_ptr<table> make_table(bool is_inline = false);

TOML_NAMESPACE_END
} // namespace toml