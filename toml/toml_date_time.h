#pragma once

#include "toml_base.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

struct local_date
{
    uint16_t year{0};
    uint8_t month{0};
    uint8_t day{0};
};

struct local_time
{
    uint8_t hour{0};        // from 0 - 59.
    uint8_t minute{0};      // from 0 - 59.
    uint8_t second{0};      // from 0 - 60.
    uint32_t nanosecond{0}; // from 0 - 999999999.
};

struct time_offset
{
    int16_t minute_offset{0};

    time_offset() = default;

    time_offset(int16_t minutes)
        : minute_offset(minutes) {}

    time_offset(int8_t h, int8_t m) noexcept
        : minute_offset{static_cast<int16_t>(h * 60 + m)} {}
};

struct local_date_time : local_date, local_time
{
    local_date_time() = default;

    local_date_time(local_date &&date, local_time &&time)
        : local_date(date),
          local_time(time) {}
};

struct offset_date_time : local_date_time, time_offset
{
    offset_date_time() = default;

    offset_date_time(local_date &&date, local_time &&time, time_offset &&offset)
        : local_date_time(std::move(date), std::move(time)),
          time_offset(offset) {}

    offset_date_time(local_date_time &&date_time, time_offset &&offset)
        : local_date_time(date_time),
          time_offset(offset) {}
};

TOML_NAMESPACE_END
} // namespace toml