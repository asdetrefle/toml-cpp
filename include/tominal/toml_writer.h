#pragma once

#include <algorithm>
#include "toml_value.h"
#include "toml_array.h"
#include "toml_table.h"
#include "toml_node_view.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

class toml_writer
{
public:
    /**
     * Construct a toml_writer that will write to the given stream
     */
    toml_writer(std::ostream &s, size_t indent_space = 4)
        : stream_(s),
          indent_(indent_space, ' '),
          has_naked_endline_(false) {}

public:
    /**
     * Output a node value of the TOML tree.
     */
    template <class T>
    void visit(const value<T> &v, bool = false)
    {
        write(v);
    }

    /**
     * Output a table element of the TOML tree
     */
    void visit(const table &t, bool in_array = false)
    {
        write_table_header(in_array);
        std::vector<std::string> values;
        std::vector<std::string> tables;

        for (const auto &i : t)
        {
            if (i.second->is_table())
            {
                tables.push_back(i.first);
            }
            else
            {
                values.push_back(i.first);
            }
        }

        std::sort(values.begin(), values.end(), [&](const auto &lhs, const auto &rhs) {
            return t.at(lhs)->is_table_array() < t.at(rhs)->is_table_array() ? true : lhs < rhs;
        });

        for (size_t i = 0; i < values.size(); ++i)
        {
            path_.push_back(values[i]);

            if (i > 0)
            {
                endline();

                if (t.at(values[i])->is_table_array())
                    stream_ << "\n";
            }

            write_table_item_header(*(t.at(values[i])));

            t.at(values[i])->accept(*this, false);

            path_.pop_back();
        }

        for (unsigned int i = 0; i < tables.size(); ++i)
        {
            path_.push_back(tables[i]);

            if (values.size() > 0 || i > 0)
                endline();

            write_table_item_header(*(t.at(tables[i])));
            t.at(tables[i])->accept(*this, false);
            path_.pop_back();
        }

        endline();
        stream_ << "\n";
    }

    /**
     * Output an array element of the TOML tree
     */
    void visit(const array &a, bool = false)
    {
        if (a.is_table_array())
        {
            for (size_t i = 0; i < a.size(); ++i)
            {
                a.at(i)->as<table>()->accept(*this, true);
            }
        }
        else
        {
            write("[");

            for (unsigned int i = 0; i < a.get().size(); ++i)
            {
                if (i > 0)
                    write(", ");

                if (auto n = a.at(i); n->is_array())
                {
                    n->as<array>()->accept(*this, true);
                }
                else
                {
                    n->accept(*this, true);
                }
            }

            write("]");
        }
    }

    /**
     * Escape a string for output.
     */
    static std::string escape_string(const std::string &str)
    {
        std::string res;
        for (auto it = str.begin(); it != str.end(); ++it)
        {
            if (*it == '\b')
            {
                res += "\\b";
            }
            else if (*it == '\t')
            {
                res += "\\t";
            }
            else if (*it == '\n')
            {
                res += "\\n";
            }
            else if (*it == '\f')
            {
                res += "\\f";
            }
            else if (*it == '\r')
            {
                res += "\\r";
            }
            else if (*it == '"')
            {
                res += "\\\"";
            }
            else if (*it == '\\')
            {
                res += "\\\\";
            }
            else if (static_cast<uint32_t>(*it) <= UINT32_C(0x001f))
            {
                res += "\\u";
                std::stringstream ss;
                ss << std::hex << static_cast<uint32_t>(*it);
                res += ss.str();
            }
            else
            {
                res += *it;
            }
        }
        return res;
    }

protected:
    /**
     * Write out a string.
     */
    void write(const value<std::string> &v)
    {
        write("\"");
        write(escape_string(v.get()));
        write("\"");
    }

    /**
     * Write out a double.
     */
    void write(const value<double> &v)
    {
        std::stringstream ss;
        ss << std::showpoint
           << std::setprecision(std::numeric_limits<double>::max_digits10)
           << v.get();

        auto double_str = ss.str();
        auto pos = double_str.find("e0");
        if (pos != std::string::npos)
            double_str.replace(pos, 2, "e");
        pos = double_str.find("e-0");
        if (pos != std::string::npos)
            double_str.replace(pos, 3, "e-");

        stream_ << double_str;
        has_naked_endline_ = false;
    }

    /**
     * Write out an integer, local_date, local_time, local_date_time, or
     * offset_date_time.
     */
    //        is_one_of<T, int64_t, local_date, local_time, local_date_time,
    //     offset_date_time>::value>::type
    template <class T>
    typename std::enable_if_t<is_one_of_v<T, int64_t, local_date, local_time,
                                          local_date_time, offset_date_time>>
    write(const value<T> &v)
    {
        write(v.get());
    }

    /**
     * Write out a boolean.
     */
    void write(const value<bool> &v)
    {
        write((v.get() ? "true" : "false"));
    }

    /**
     * Write out the header of a table.
     */
    void write_table_header(bool in_array = false)
    {
        if (!path_.empty())
        {
            indent();

            write("[");

            if (in_array)
            {
                write("[");
            }

            for (unsigned int i = 0; i < path_.size(); ++i)
            {
                if (i > 0)
                {
                    write(".");
                }

                if (path_[i].find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcde"
                                               "fghijklmnopqrstuvwxyz0123456789"
                                               "_-") == std::string::npos)
                {
                    write(path_[i]);
                }
                else
                {
                    write("\"");
                    write(escape_string(path_[i]));
                    write("\"");
                }
            }

            if (in_array)
            {
                write("]");
            }

            write("]");
            endline();
        }
    }

    /**
     * Write out the identifier for an item in a table.
     */
    void write_table_item_header(const node &b)
    {
        if (!b.is_table() && !b.is_table_array())
        {
            indent();

            if (path_.back().find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcde"
                                               "fghijklmnopqrstuvwxyz0123456789"
                                               "_-") == std::string::npos)
            {
                write(path_.back());
            }
            else
            {
                write("\"");
                write(escape_string(path_.back()));
                write("\"");
            }

            write(" = ");
        }
    }

private:
    /**
     * Indent the proper number of tabs given the size of
     * the path.
     */
    void indent()
    {
        for (std::size_t i = 1; i < path_.size(); ++i)
            write(indent_);
    }

    /**
     * Write a value out to the stream.
     */
    template <class T>
    void write(const T &v)
    {
        stream_ << v;
        has_naked_endline_ = false;
    }

    /**
     * Write an endline out to the stream
     */
    void endline()
    {
        if (!has_naked_endline_)
        {
            stream_ << "\n";
            has_naked_endline_ = true;
        }
    }

private:
    std::ostream &stream_;
    const std::string indent_;
    std::vector<std::string> path_;
    bool has_naked_endline_;
};

template <class T>
std::ostream &operator<<(std::ostream &stream, const value<T> &v)
{
    toml_writer writer{stream};
    v.accept(writer);
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const table &t)
{
    toml_writer writer{stream};
    t.accept(writer);
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const array &a)
{
    toml_writer writer{stream};
    a.accept(writer);
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, const node &b)
{
    toml_writer writer{stream};
    b.accept(writer);
    return stream;
}

inline std::ostream &operator<<(std::ostream &stream, node_view b)
{
    toml_writer writer{stream};
    b.accept(writer);
    return stream;
}

TOML_NAMESPACE_END
} // namespace toml