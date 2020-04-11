#pragma once

#define TOML_STANDARD toml_1_0_0_rc1
#define TOML_INLINE_NAMESPACE_NAME lts_2020_04_09

#define TOML_NAMESPACE_BEGIN                    \
    inline namespace TOML_INLINE_NAMESPACE_NAME \
    {
#define TOML_NAMESPACE_END }

namespace toml
{
TOML_NAMESPACE_BEGIN

enum class base_type : uint8_t
{
    None = 0,
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

class node;

template <typename T>
class node_view;

template <typename T>
class value;

class parse_result;
TOML_NAMESPACE_END
} // namespace toml

#include "toml/toml_base.h"
#include "toml/toml_date_time.h"
#include "toml/toml_node.h"
#include "toml/toml_node_view.h"
#include "toml/toml_value.h"
#include "toml/toml_array.h"
#include "toml/toml_table.h"
#include "toml/toml_parser.h"