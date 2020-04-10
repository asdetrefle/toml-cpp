#pragma once

#include <fstream>
#include <stdexcept>
#include <variant>
#include "toml_base.h"
#include "toml_table.h"
#include "toml_node_view.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

struct source_position
{
    size_t line;   // The line number starting at 1
    size_t column; // The column number starting at 1
};

struct source_region
{
    source_position begin; //The beginning of the region
    source_position end;   // The end of the region
};

class parse_error : public std::runtime_error
{
public:
    parse_error(const char *desc) noexcept
        : std::runtime_error{desc} {}

    parse_error(const std::string &desc) noexcept
        : std::runtime_error{desc.data()} {}

    parse_error(const char *desc, const source_region &src) noexcept
        : std::runtime_error{desc},
          source_{src} {}

    std::string_view description() const noexcept
    {
        return std::string_view{what()};
    }

    const source_region &
    source() const noexcept
    {
        return source_;
    }

private:
    source_region source_;
}; // namespace toml

class parse_result
{
public:
    bool is_ok() const noexcept
    {
        return !is_error_;
    }

    bool is_err() const noexcept
    {
        return is_error_;
    }

    explicit operator bool() const noexcept
    {
        return !is_error_;
    }

    node_view<node> ok() noexcept
    {
        if (is_ok())
        {
            return std::get<node_view<node>>(result_);
        }
        else
        {
            return {nullptr};
        }
    }

    const parse_error &error() const
    {
        if (is_err())
        {
            return std::get<parse_error>(result_);
        }
        else
        {
            throw std::runtime_error("parse result is not an error");
        }
    }

    operator node_view<node>() noexcept
    {
        return ok();
    }

    explicit operator const parse_error &() const
    {
        return error();
    }

    parse_result(std::shared_ptr<table> &&tbl) noexcept
        : is_error_{false}
    {
        node_view<node> view{tbl};
        result_.emplace<node_view<node>>(view);
    }

    parse_result(parse_error &&err) noexcept
        : is_error_{true},
          result_{std::in_place_type<parse_error>, err} {}

private:
    bool is_error_;
    std::variant<node_view<node>, parse_error> result_;
};

parse_result parse_file(const std::string &file_path)
{
    std::ifstream file{file_path};

    if (0 && !file.is_open())
    {
        return {parse_error(file_path + " could not be opened for parsing")};
    }
    else
    {
        auto tbl = make_table();
        tbl->insert_or_assign("haha", make_value(1));

        auto array = make_array();
        array->emplace_back<double>(0.5);

        tbl->insert_or_assign("hoho", array);
        return {std::move(tbl)};
    }
}
TOML_NAMESPACE_END
} // namespace toml