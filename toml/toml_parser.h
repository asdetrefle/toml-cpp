#pragma once

#include <stdexcept>
#include "toml_base.h"
#include "toml_table.h"
#include "toml_node_view.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

class parse_error : public std::runtime_error
{
public:
    parse_error(const char *desc, const source_region &src) noexcept
        : parse_error{desc, source_region{src}} {}

    parse_error(const char *desc, const source_position &position, const source_path_ptr &path = {}) noexcept
        : parse_error{desc, source_region{position, position, path}} {}

    std::string_view description() const noexcept
    {
        return std::string_view{what()};
    }

    const source_region &source() const noexcept
    {
        return source_;
    }

private:
    source_region source_;
};

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

    node_view<node> view() noexcept
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

    const parse_error &error()
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
        return view();
    }

    explicit operator const parse_error &() const
    {
        return error();
    }

    explicit parse_result(std::shared_ptr<table> &&tbl) noexcept
        : is_error_{false}
    {
        result_.emplace<node_view<node>>(std::forward(tbl));
    }

    explicit parse_result(parse_error &&err) noexcept
        : is_error_{true}
    {
        result_.emplace(std::forward(err));
    }

private:
    bool is_error_;
    std::variant<node_view<node>, parse_error> result_;
};

parse_result parse_file(const std::string &file_path)
{
    std::ifstream file{file_path};

    if (!file.is_open())
        throw parse_exception{file_path + " could not be opened for parsing"};

}
TOML_NAMESPACE_END
} // namespace toml