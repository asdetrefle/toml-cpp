#pragma once

#include <iomanip>

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

class fill_guard
{
public:
    fill_guard(std::ostream &os)
        : os_(os),
          fill_{os.fill()} {}

    ~fill_guard()
    {
        os_.fill(fill_);
    }

private:
    std::ostream &os_;
    std::ostream::char_type fill_;
};

inline std::ostream &operator<<(std::ostream &os, const local_date &dt)
{
    fill_guard g{os};
    os.fill('0');

    using std::setw;
    os << setw(4) << static_cast<uint16_t>(dt.year) << '-'
       << setw(2) << static_cast<uint16_t>(dt.month) << '-'
       << setw(2) << static_cast<uint16_t>(dt.day);

    return os;
}

inline std::ostream &operator<<(std::ostream &os, const local_time &ltime)
{
    fill_guard g{os};
    os.fill('0');

    using std::setw;
    os << setw(2) << static_cast<uint16_t>(ltime.hour) << ':'
       << setw(2) << static_cast<uint16_t>(ltime.minute) << ':'
       << setw(2) << static_cast<uint16_t>(ltime.second);

    if (ltime.nanosecond > 0)
    {
        os << '.' << setw(9) << ltime.nanosecond;
    }

    return os;
}

inline std::ostream &operator<<(std::ostream &os, const time_offset &offset)
{
    fill_guard g{os};
    os.fill('0');

    using std::setw;

    if (offset.minute_offset != 0)
    {
        if (offset.minute_offset > 0)
        {
            os << '+';
        }
        else
        {
            os << '-';
        }
        auto [hour, minute] = std::div(std::abs(offset.minute_offset), 60);
        os << setw(2) << hour << ':' << setw(2) << std::abs(minute);
    }
    else
    {
        os << 'Z';
    }

    return os;
}

inline std::ostream &operator<<(std::ostream &os, const local_date_time &dt)
{
    return os << static_cast<const local_date &>(dt) << 'T'
              << static_cast<const local_time &>(dt);
}

inline std::ostream &operator<<(std::ostream &os, const offset_date_time &dt)
{
    return os << static_cast<const local_date_time &>(dt)
              << static_cast<const time_offset &>(dt);
}

TOML_NAMESPACE_END
} // namespace toml