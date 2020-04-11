#pragma once

#include "toml_base.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

struct local_date
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
};

struct local_time
{
    uint8_t hour;        // from 0 - 59.
    uint8_t minute;      // from 0 - 59.
    uint8_t second;      // from 0 - 60.
    uint32_t nanosecond; // from 0 - 999999999.
};

struct time_offset
{
    uint16_t hour_offset = 0;
    uint16_t minute_offset = 0;
};

struct local_date_time : local_date, local_time
{
};

struct offset_date_time : local_date_time, time_offset
{
    static inline struct offset_date_time from_zoned(const struct tm &t)
    {
        offset_date_time dt;
        dt.year = t.tm_year + 1900;
        dt.month = t.tm_mon + 1;
        dt.day = t.tm_mday;
        dt.hour = t.tm_hour;
        dt.minute = t.tm_min;
        dt.second = t.tm_sec;

        char buf[16];
        strftime(buf, 16, "%z", &t);

        int offset = std::stoi(buf);
        dt.hour_offset = offset / 100;
        dt.minute_offset = offset % 100;
        return dt;
    }
};

TOML_NAMESPACE_END
}