#pragma once

#include <optional>
#include <string>
#include <type_traits>

#define TOML_STANDARD toml_1_0_0_rc1
#define TOML_INLINE_NAMESPACE_NAME lts_2020_04_09

#define TOML_NAMESPACE_BEGIN                    \
    inline namespace TOML_INLINE_NAMESPACE_NAME \
    {
#define TOML_NAMESPACE_END }

namespace toml
{
inline namespace TOML_STANDARD
{
enum class base_type
{
    None,
    String,
    Integer,
    Float,
    Boolean,
    OffsetDateTime,
    LocalDateTime,
    LocalDate,
    LocalTime,
    Array,
    Table,
    TableArray,
};

class local_date;
class local_time;
class local_date_time;
class time_offset;
class offset_date_time;

class array;
class table;
} // namespace TOML_STANDARD

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
    static constexpr auto value = base_type::Unknown;
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

class node;

template <typename>
class node_view;

template <typename>
class value;

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
    using type = int64_t;
    static constexpr auto value = base_type::Integer;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
{
    using type = typename T;
    static constexpr auto value = base_type::Float;
};

template <class T>
struct value_type_traits<T, typename std::enable_if_t<is_one_of_v<T, std::string_view, const char *>>>
{
    using type = std::string;
    static constexpr auto value = base_type::String;
};

template <typename T>
using map = std::map<string, T, std::less<>>;

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
inline constexpr bool is_value_or_node = (base_type_traits<T>::value != base_type::Unknown);

template <typename T>
inline constexpr bool is_value = (base_type_traits<T>::value != base_type::Unknown) &&
                                 (base_type_traits<T>::value != base_type::Table) &&
                                 (base_type_traits<T>::value != base_type::Array);

template <typename T>
inline constexpr bool is_value_promotable = (value_type_traits<T>::value != base_type::Unknown) &&
                                            (value_type_traits<T>::value != base_type::Table) &&
                                            (value_type_traits<T>::value != base_type::Array);
} // namespace toml
}