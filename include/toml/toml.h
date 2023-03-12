#pragma once

#define TOML_STANDARD toml_1_0_0_rc2
#define TOML_INLINE_NAMESPACE_NAME lts_2023_03_12

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

#include "base.h"
#include "date_time.h"
#include "node.h"
#include "value.h"
#include "array.h"
#include "table.h"
#include "node_view.h"
#include "parser.h"
#include "writer.h"