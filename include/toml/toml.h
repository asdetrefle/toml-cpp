#pragma once

#define TOML_STANDARD toml_1_0_0_rc1
#define TOML_INLINE_NAMESPACE_NAME lts_2020_04_09

#define TOML_NAMESPACE_BEGIN                    \
    inline namespace TOML_INLINE_NAMESPACE_NAME \
    {
#define TOML_NAMESPACE_END }

#include <cstdint>

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

struct local_date;
struct local_time;
struct local_date_time;
struct time_offset;
struct offset_date_time;

class array;
class table;

class node;
class node_view;

template <typename T>
class value;

class parse_result;
TOML_NAMESPACE_END
} // namespace toml

#include "toml_base.h"
#include "toml_date_time.h"
#include "toml_node.h"
#include "toml_value.h"
#include "toml_array.h"
#include "toml_table.h"
#include "toml_node_view.h"
#include "toml_parser.h"
#include "toml_writer.h"