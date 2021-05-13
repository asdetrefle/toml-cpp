#pragma once

#include <cctype>
#include <fstream>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <variant>

#include "toml_base.h"
#include "toml_date_time.h"
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

class parse_error : public std::runtime_error
{
public:
    parse_error(const std::string &desc) noexcept
        : std::runtime_error{desc.data()} {}

    parse_error(const std::string &desc, size_t line_number) noexcept
        : std::runtime_error{desc.data()},
          source_{line_number, 0} {}

    std::string_view description() const noexcept
    {
        return std::string_view{what()};
    }

    const source_position &source() const noexcept
    {
        return source_;
    }

private:
    source_position source_;
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

    node_view ok() noexcept
    {
        if (is_ok())
        {
            return std::get<node_view>(result_);
        }
        else
        {
            return {nullptr};
        }
    }

    const parse_error &err() const
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

    operator node_view() noexcept
    {
        return ok();
    }

    explicit operator const parse_error &() const
    {
        return err();
    }

    parse_result(std::shared_ptr<table> &&tbl) noexcept
        : is_error_{false},
          result_{std::in_place_type<node_view>, tbl} {}

    parse_result(const parse_error &err) noexcept
        : is_error_{true},
          result_{std::in_place_type<parse_error>, err} {}

private:
    bool is_error_;
    std::variant<node_view, parse_error> result_;
};

/**
 * Helper object for consuming expected characters.
 */
class consumer
{
public:
    consumer(std::string::iterator &it,
             const std::string::iterator &end,
             std::function<void()> on_error)
        : it_(it),
          end_(end),
          on_error_(on_error) {}

    void operator()(char c)
    {
        if (it_ == end_ || *it_ != c)
            on_error_();
        ++it_;
    }

    template <std::size_t N>
    void operator()(const char (&str)[N])
    {
        std::for_each(std::begin(str), std::end(str) - 1,
                      [&](char c) { (*this)(c); });
    }

    void eat_either(char a, char b)
    {
        if (it_ == end_ || (*it_ != a && *it_ != b))
            on_error_();
        ++it_;
    }

    int eat_digits(int len)
    {
        int val = 0;
        for (int i = 0; i < len; ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(*it_)) || it_ == end_)
                on_error_();
            val = 10 * val + (*it_++ - '0');
        }
        return val;
    }

private:
    std::string::iterator &it_;
    const std::string::iterator &end_;
    std::function<void()> on_error_;
};

// replacement for std::getline to handle incorrectly line-ended files
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
namespace detail
{
inline std::istream &getline(std::istream &input, std::string &line)
{
    line.clear();

    std::istream::sentry sentry{input, true};
    auto sb = input.rdbuf();

    while (true)
    {
        auto c = sb->sbumpc();
        if (c == '\r')
        {
            if (sb->sgetc() == '\n')
                c = sb->sbumpc();
        }

        if (c == '\n')
            return input;

        if (c == std::istream::traits_type::eof())
        {
            if (line.empty())
                input.setstate(std::ios::eofbit);
            return input;
        }

        line.push_back(static_cast<char>(c));
    }
}
} // namespace detail

/**
 * The parser class.
 */
class parser
{
public:
    /**
     * Parsers are constructed from streams.
     */
    parser(std::istream &stream)
        : input_(stream) {}

    parser &operator=(const parser &parser) = delete;

    /**
     * Parses the stream this parser was created on until EOF.
     * @throw parse_exception if there are errors in parsing
     */
    std::shared_ptr<table> parse()
    {
        std::shared_ptr<table> root = make_table();

        table *curr_table = root.get();

        while (detail::getline(input_, line_))
        {
            line_number_++;
            auto it = line_.begin();
            auto end = line_.end();
            consume_whitespace(it, end);
            if (it == end || *it == '#')
                continue;
            if (*it == '[')
            {
                curr_table = root.get();
                parse_table(it, end, curr_table);
            }
            else
            {
                parse_key_value(it, end, curr_table);
                consume_whitespace(it, end);
                eol_or_comment(it, end);
            }
        }
        return root;
    }

private:
#if defined _MSC_VER
    __declspec(noreturn)
#elif defined __GNUC__
    __attribute__((noreturn))
#endif
        void throw_parse_exception(const std::string &err)
    {
        throw parse_error{err, line_number_};
    }

    void parse_table(std::string::iterator &it,
                     const std::string::iterator &end, table *&curr_table)
    {
        // remove the beginning keytable marker
        ++it;
        if (it == end)
            throw_parse_exception("Unexpected end of table");
        if (*it == '[')
            parse_table_array(it, end, curr_table);
        else
            parse_single_table(it, end, curr_table);
    }

    void parse_single_table(std::string::iterator &it,
                            const std::string::iterator &end,
                            table *&curr_table)
    {
        if (it == end || *it == ']')
            throw_parse_exception("Table name cannot be empty");

        std::string full_table_name;
        bool inserted = false;

        auto key_end = [](char c) { return c == ']'; };

        auto key_part_handler = [&](const std::string &part) {
            if (part.empty())
                throw_parse_exception("Empty component of table name");

            if (!full_table_name.empty())
                full_table_name += '.';
            full_table_name += part;

            if (curr_table->contains(part))
            {
                auto b = curr_table->at(part);
                if (b->is<table>())
                    curr_table = static_cast<table *>(b.get());
                else if (b->is_table_array())
                    curr_table = b->as<array>()->back()->as<table>().get();
                else
                    throw_parse_exception("Key " + full_table_name + "already exists as a value");
            }
            else
            {
                inserted = true;
                curr_table->emplace(part, make_table());
                curr_table = static_cast<table *>(curr_table->at(part).get());
            }
        };

        key_part_handler(parse_key(it, end, key_end, key_part_handler));

        if (it == end)
            throw_parse_exception(
                "Unterminated table declaration; did you forget a ']'?");

        if (*it != ']')
        {
            std::string errmsg{"Unexpected character in table definition: "};
            errmsg += '"';
            errmsg += *it;
            errmsg += '"';
            throw_parse_exception(errmsg);
        }

        // table already existed
        if (!inserted)
        {
            auto is_value = [](const std::pair<const std::string &,
                                               const std::shared_ptr<node> &> &p) {
                return p.second->is_value();
            };

            // if there are any values, we can't add values to this table
            // since it has already been defined. If there aren't any
            // values, then it was implicitly created by something like
            // [a.b]
            if (curr_table->empty() || std::any_of(curr_table->begin(), curr_table->end(),
                                                   is_value))
            {
                throw_parse_exception("Redefinition of table " + full_table_name);
            }
        }

        ++it;
        consume_whitespace(it, end);
        eol_or_comment(it, end);
    }

    void parse_table_array(std::string::iterator &it,
                           const std::string::iterator &end, table *&curr_table)
    {
        ++it;
        if (it == end || *it == ']')
            throw_parse_exception("Table array name cannot be empty");

        auto key_end = [](char c) { return c == ']'; };

        std::string full_ta_name;
        auto key_part_handler = [&](const std::string &part) {
            if (part.empty())
                throw_parse_exception("Empty component of table array name");

            if (!full_ta_name.empty())
                full_ta_name += '.';
            full_ta_name += part;

            if (curr_table->contains(part))
            {
                auto b = curr_table->at(part);

                // if this is the end of the table array name, add an
                // element to the table array that we just looked up,
                // provided it was not declared inline
                if (it != end && *it == ']')
                {
                    if (!b->is_table_array())
                    {
                        throw_parse_exception("key `" + full_ta_name + "` is not a table array");
                    }

                    auto v = b->as<array>();
                    for (auto it = v->begin(); it != v->end(); ++it)
                    {
                        if ((*it)->as<table>()->is_inline())
                        {
                            throw_parse_exception("static table array `" + full_ta_name + "` cannot be appended to");
                        }
                    }

                    v->push_back(make_table());
                    curr_table = v->back()->as<table>().get();
                }
                // otherwise, just keep traversing down the key name
                else
                {
                    if (b->is<table>())
                        curr_table = static_cast<table *>(b.get());
                    else if (b->is_table_array())
                        curr_table = b->as<array>()->back()->as<table>().get();
                    else
                        throw_parse_exception("Key " + full_ta_name + " already exists as a value");
                }
            }
            else
            {
                // if this is the end of the table array name, add a new
                // table array and a new table inside that array for us to
                // add keys to next
                if (it != end && *it == ']')
                {
                    auto arr = make_array();
                    arr->push_back(make_table());
                    auto [it, success] = curr_table->emplace(part, std::move(arr));
                    curr_table = it->second->as<array>()->back()->as<table>().get();
                }
                // otherwise, create the implicitly defined table and move
                // down to it
                else
                {
                    curr_table->emplace(part, make_table());
                    curr_table = static_cast<table *>(curr_table->at(part).get());
                }
            }
        };

        key_part_handler(parse_key(it, end, key_end, key_part_handler));

        // consume the last "]]"
        auto eat = consumer(it, end, [&]() {
            throw_parse_exception("Unterminated table array name");
        });
        eat(']');
        eat(']');

        consume_whitespace(it, end);
        eol_or_comment(it, end);
    }

    void parse_key_value(std::string::iterator &it, std::string::iterator &end,
                         table *curr_table)
    {
        auto key_end = [](char c) { return c == '='; };

        auto key_part_handler = [&](const std::string &part) {
            // two cases: this key part exists already, in which case it must
            // be a table, or it doesn't exist in which case we must create
            // an implicitly defined table
            if (curr_table->contains(part))
            {
                auto val = curr_table->at(part);
                if (val->is<table>())
                {
                    curr_table = static_cast<table *>(val.get());
                }
                else
                {
                    throw_parse_exception("Key " + part + " already exists as a value");
                }
            }
            else
            {
                auto [it, success] = curr_table->emplace(part, make_table());
                curr_table = it->second->as<table>().get();
            }
        };

        auto key = parse_key(it, end, key_end, key_part_handler);

        if (curr_table->contains(key))
            throw_parse_exception("Key " + key + " already present");
        if (it == end || *it != '=')
            throw_parse_exception("Value must follow after a '='");
        ++it;
        consume_whitespace(it, end);
        curr_table->emplace(key, parse_value(it, end));
        consume_whitespace(it, end);
    }

    template <class KeyEndFinder, class KeyPartHandler>
    std::string parse_key(std::string::iterator &it, const std::string::iterator &end,
                          KeyEndFinder &&key_end, KeyPartHandler &&key_part_handler)
    {
        // parse the key as a series of one or more simple-keys joined with '.'
        while (it != end && !key_end(*it))
        {
            auto part = parse_simple_key(it, end);
            consume_whitespace(it, end);

            if (it == end || key_end(*it))
            {
                return part;
            }

            if (*it != '.')
            {
                std::string errmsg{"Unexpected character in key: "};
                errmsg += '"';
                errmsg += *it;
                errmsg += '"';
                throw_parse_exception(errmsg);
            }

            key_part_handler(part);

            // consume the dot
            ++it;
        }

        throw_parse_exception("Unexpected end of key");
    }

    std::string parse_simple_key(std::string::iterator &it,
                                 const std::string::iterator &end)
    {
        consume_whitespace(it, end);

        if (it == end)
            throw_parse_exception("Unexpected end of key (blank key?)");

        if (*it == '"' || *it == '\'')
        {
            return string_literal(it, end, *it);
        }
        else
        {
            auto bke = std::find_if(it, end, [](char c) {
                return c == '.' || c == '=' || c == ']';
            });
            return parse_bare_key(it, bke);
        }
    }

    std::string parse_bare_key(std::string::iterator &it,
                               const std::string::iterator &end)
    {
        if (it == end)
        {
            throw_parse_exception("Bare key missing name");
        }

        auto key_end = end;
        --key_end;
        consume_backwards_whitespace(key_end, it);
        ++key_end;
        std::string key{it, key_end};

        if (std::find(it, key_end, '#') != key_end)
        {
            throw_parse_exception("Bare key " + key + " cannot contain #");
        }

        if (std::find_if(it, key_end,
                         [](char c) { return c == ' ' || c == '\t'; }) != key_end)
        {
            throw_parse_exception("Bare key " + key + " cannot contain whitespace");
        }

        if (std::find_if(it, key_end,
                         [](char c) { return c == '[' || c == ']'; }) != key_end)
        {
            throw_parse_exception("Bare key " + key + " cannot contain '[' or ']'");
        }

        it = end;
        return key;
    }

    enum class numeric_type : uint8_t
    {
        None = 0,
        LocalTime,
        LocalDate,
        LocalDateTime,
        OffsetDateTime,
        Integer,
        Float,
    };

    std::shared_ptr<node> parse_value(std::string::iterator &it,
                                      std::string::iterator &end)
    {
        if (*it == '[')
        {
            // parse array
            return parse_array(it, end);
        }
        else if (*it == '{')
        {
            // parse inline table
            return parse_inline_table(it, end);
        }
        else if (*it == '"' || *it == '\'')
        {
            // STRING:
            return parse_string(it, end);
        }
        else if (*it == 't' || *it == 'f')
        {
            // BOOL;
            return parse_bool(it, end);
        }
        else
        {
            auto val_end = std::find_if(
                it, end, [](char c) { return c == ',' || c == ']' || c == '#'; });

            numeric_type type = determine_numeric_type(it, val_end);

            switch (type)
            {
            case numeric_type::LocalTime:
                return parse_time(it, end);
            case numeric_type::LocalDate:
            case numeric_type::LocalDateTime:
            case numeric_type::OffsetDateTime:
                return parse_date(it, end);
            case numeric_type::Integer:
            case numeric_type::Float:
                return parse_number(it, end);
            default:
                throw_parse_exception("Failed to parse value");
            }
        }
    }

    numeric_type determine_numeric_type(const std::string::iterator &it,
                                        const std::string::iterator &end)
    {
        if (it == end)
        {
            return numeric_type::None;
        }
        else if (is_time(it, end))
        {
            return numeric_type::LocalTime;
        }
        else if (auto date_type = determine_date_type(it, end); date_type != numeric_type::None)
        {
            return date_type;
        }
        else if (std::isdigit(static_cast<unsigned char>(*it)) || *it == '-' || *it == '+' ||
                 (*it == 'i' && it + 1 != end && it[1] == 'n' && it + 2 != end && it[2] == 'f') ||
                 (*it == 'n' && it + 1 != end && it[1] == 'a' && it + 2 != end && it[2] == 'n'))
        {
            return determine_number_type(it, end);
        }
        else
        {
            return numeric_type::None;
        }
    }

    numeric_type determine_number_type(const std::string::iterator &it,
                                       const std::string::iterator &end)
    {
        // determine if we are an integer or a float
        auto check_it = it;
        if (*check_it == '-' || *check_it == '+')
            ++check_it;

        if (check_it == end)
            return numeric_type::None;

        if (*check_it == 'i' || *check_it == 'n')
            return numeric_type::Float;

        while (check_it != end && std::isdigit(static_cast<unsigned char>(*check_it)))
            ++check_it;
        if (check_it != end && *check_it == '.')
        {
            ++check_it;
            while (check_it != end && std::isdigit(static_cast<unsigned char>(*check_it)))
                ++check_it;
            return numeric_type::Float;
        }
        else
        {
            return numeric_type::Integer;
        }
    }

    std::shared_ptr<value<std::string>> parse_string(std::string::iterator &it,
                                                     std::string::iterator &end)
    {
        auto delim = *it;
        assert(delim == '"' || delim == '\'');

        // end is non-const here because we have to be able to potentially
        // parse multiple lines in a string, not just one
        auto check_it = it;
        ++check_it;
        if (check_it != end && *check_it == delim)
        {
            ++check_it;
            if (check_it != end && *check_it == delim)
            {
                it = ++check_it;
                return parse_multiline_string(it, end, delim);
            }
        }
        return make_value(string_literal(it, end, delim));
    }

    std::shared_ptr<value<std::string>>
    parse_multiline_string(std::string::iterator &it,
                           std::string::iterator &end, char delim)
    {
        std::stringstream ss;

        auto is_ws = [](char c) { return c == ' ' || c == '\t'; };

        bool consuming = false;
        std::shared_ptr<value<std::string>> ret;

        auto handle_line = [&](std::string::iterator &local_it,
                               std::string::iterator &local_end) {
            if (consuming)
            {
                local_it = std::find_if_not(local_it, local_end, is_ws);

                // whole line is whitespace
                if (local_it == local_end)
                    return;
            }

            consuming = false;

            while (local_it != local_end)
            {
                // handle escaped characters
                if (delim == '"' && *local_it == '\\')
                {
                    auto check = local_it;
                    // check if this is an actual escape sequence or a
                    // whitespace escaping backslash
                    ++check;
                    consume_whitespace(check, local_end);
                    if (check == local_end)
                    {
                        consuming = true;
                        break;
                    }

                    ss << parse_escape_code(local_it, local_end);
                    continue;
                }

                // if we can end the string
                if (std::distance(local_it, local_end) >= 3)
                {
                    auto check = local_it;
                    // check for """
                    if (*check++ == delim && *check++ == delim && *check++ == delim)
                    {
                        local_it = check;
                        ret = make_value<std::string>(ss.str());
                        break;
                    }
                }

                ss << *local_it++;
            }
        };

        // handle the remainder of the current line
        handle_line(it, end);
        if (ret)
            return ret;

        // start eating lines
        while (detail::getline(input_, line_))
        {
            ++line_number_;

            it = line_.begin();
            end = line_.end();

            handle_line(it, end);

            if (ret)
                return ret;

            if (!consuming)
                ss << std::endl;
        }

        throw_parse_exception("Unterminated multi-line basic string");
    }

    std::string string_literal(std::string::iterator &it,
                               const std::string::iterator &end, char delim)
    {
        ++it;
        std::string val;
        while (it != end)
        {
            // handle escaped characters
            if (delim == '"' && *it == '\\')
            {
                val += parse_escape_code(it, end);
            }
            else if (*it == delim)
            {
                ++it;
                consume_whitespace(it, end);
                return val;
            }
            else
            {
                val += *it++;
            }
        }
        throw_parse_exception("Unterminated string literal");
    }

    std::string parse_escape_code(std::string::iterator &it,
                                  const std::string::iterator &end)
    {
        ++it;
        if (it == end)
            throw_parse_exception("Invalid escape sequence");
        char value;
        if (*it == 'b')
        {
            value = '\b';
        }
        else if (*it == 't')
        {
            value = '\t';
        }
        else if (*it == 'n')
        {
            value = '\n';
        }
        else if (*it == 'f')
        {
            value = '\f';
        }
        else if (*it == 'r')
        {
            value = '\r';
        }
        else if (*it == '"')
        {
            value = '"';
        }
        else if (*it == '\\')
        {
            value = '\\';
        }
        else if (*it == 'u' || *it == 'U')
        {
            return parse_unicode(it, end);
        }
        else
        {
            throw_parse_exception("Invalid escape sequence");
        }
        ++it;
        return std::string(1, value);
    }

    std::string parse_unicode(std::string::iterator &it,
                              const std::string::iterator &end)
    {
        bool large = *it++ == 'U';
        auto codepoint = parse_hex(it, end, large ? 0x10000000 : 0x1000);

        if ((codepoint > 0xd7ff && codepoint < 0xe000) || codepoint > 0x10ffff)
        {
            throw_parse_exception(
                "Unicode escape sequence is not a Unicode scalar value");
        }

        std::string result;
        // See Table 3-6 of the Unicode standard
        if (codepoint <= 0x7f)
        {
            // 1-byte codepoints: 00000000 0xxxxxxx
            // repr: 0xxxxxxx
            result += static_cast<char>(codepoint & 0x7f);
        }
        else if (codepoint <= 0x7ff)
        {
            // 2-byte codepoints: 00000yyy yyxxxxxx
            // repr: 110yyyyy 10xxxxxx
            //
            // 0x1f = 00011111
            // 0xc0 = 11000000
            //
            result += static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f));
            //
            // 0x80 = 10000000
            // 0x3f = 00111111
            //
            result += static_cast<char>(0x80 | (codepoint & 0x3f));
        }
        else if (codepoint <= 0xffff)
        {
            // 3-byte codepoints: zzzzyyyy yyxxxxxx
            // repr: 1110zzzz 10yyyyyy 10xxxxxx
            //
            // 0xe0 = 11100000
            // 0x0f = 00001111
            //
            result += static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f));
            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x1f));
            result += static_cast<char>(0x80 | (codepoint & 0x3f));
        }
        else
        {
            // 4-byte codepoints: 000uuuuu zzzzyyyy yyxxxxxx
            // repr: 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
            //
            // 0xf0 = 11110000
            // 0x07 = 00000111
            //
            result += static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07));
            result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f));
            result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (codepoint & 0x3f));
        }
        return result;
    }

    uint32_t parse_hex(std::string::iterator &it,
                       const std::string::iterator &end, uint32_t place)
    {
        uint32_t value = 0;
        while (place > 0)
        {
            if (it == end)
                throw_parse_exception("Unexpected end of unicode sequence");

            if (!std::isxdigit(static_cast<unsigned char>(*it)))
                throw_parse_exception("Invalid unicode escape sequence");

            value += place * hex_to_digit(*it++);
            place /= 16;
        }
        return value;
    }

    uint32_t hex_to_digit(char c)
    {
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            return static_cast<uint32_t>(c - '0');
        }
        else
        {
            return 10 + static_cast<uint32_t>(c - ((c >= 'a' && c <= 'f') ? 'a' : 'A'));
        }
    }

    std::shared_ptr<node> parse_number(std::string::iterator &it,
                                       const std::string::iterator &end)
    {
        auto check_it = it;
        auto check_end = find_end_of_number(it, end);

        auto eat_sign = [&]() {
            if (check_it != end && (*check_it == '-' || *check_it == '+'))
                ++check_it;
        };

        auto check_no_leading_zero = [&]() {
            if (check_it != end && *check_it == '0' && check_it + 1 != check_end && check_it[1] != '.')
            {
                throw_parse_exception("Numbers may not have leading zeros");
            }
        };

        auto eat_digits = [&](std::function<bool(char)> &&check_char) {
            auto beg = check_it;
            while (check_it != end && check_char(*check_it))
            {
                ++check_it;
                if (check_it != end && *check_it == '_')
                {
                    ++check_it;
                    if (check_it == end || !check_char(*check_it))
                        throw_parse_exception("Malformed number 1");
                }
            }

            if (check_it == beg)
                throw_parse_exception("Malformed number 2");
        };

        auto eat_hex = [&]() {
            eat_digits([](char c) -> bool {
                return std::isxdigit(static_cast<unsigned char>(c));
            });
        };
        auto eat_decimal = [&]() {
            eat_digits([](char c) -> bool {
                return std::isdigit(static_cast<unsigned char>(c));
            });
        };

        if (check_it != end && *check_it == '0' && check_it + 1 != check_end &&
            (check_it[1] == 'x' || check_it[1] == 'o' || check_it[1] == 'b'))
        {
            ++check_it;
            char base = *check_it;
            ++check_it;
            if (base == 'x')
            {
                eat_hex();
                return parse_int(it, check_it, 16);
            }
            else if (base == 'o')
            {
                auto start = check_it;
                eat_decimal();
                auto val = parse_int(start, check_it, 8, "0");
                it = start;
                return val;
            }
            else // if (base == 'b')
            {
                auto start = check_it;
                eat_decimal();
                auto val = parse_int(start, check_it, 2);
                it = start;
                return val;
            }
        }

        eat_sign();
        check_no_leading_zero();

        if (check_it != end && check_it + 1 != end && check_it + 2 != end)
        {
            if (check_it[0] == 'i' && check_it[1] == 'n' && check_it[2] == 'f')
            {
                auto val = std::numeric_limits<double>::infinity();
                if (*it == '-')
                    val = -val;
                it = check_it + 3;
                return make_value(std::move(val));
            }
            else if (check_it[0] == 'n' && check_it[1] == 'a' && check_it[2] == 'n')
            {
                auto val = std::numeric_limits<double>::quiet_NaN();
                if (*it == '-')
                    val = -val;
                it = check_it + 3;
                return make_value(std::move(val));
            }
        }

        eat_decimal();

        if (check_it != end && (*check_it == '.' || *check_it == 'e' || *check_it == 'E'))
        {
            bool is_exp = *check_it == 'e' || *check_it == 'E';

            ++check_it;
            if (check_it == end)
                throw_parse_exception("Floats must have trailing digits");

            auto eat_exp = [&]() {
                eat_sign();
                check_no_leading_zero();
                eat_decimal();
            };

            if (is_exp)
                eat_exp();
            else
                eat_decimal();

            if (!is_exp && check_it != end && (*check_it == 'e' || *check_it == 'E'))
            {
                ++check_it;
                eat_exp();
            }

            return parse_float(it, check_it);
        }
        else
        {
            return parse_int(it, check_it);
        }
    }

    std::shared_ptr<value<int64_t>> parse_int(std::string::iterator &it,
                                              const std::string::iterator &end,
                                              int base = 10,
                                              const char *prefix = "")
    {
        std::string v{it, end};
        v = prefix + v;
        v.erase(std::remove(v.begin(), v.end(), '_'), v.end());
        it = end;
        try
        {
            return make_value<int64_t>(std::stoll(v, nullptr, base));
        }
        catch (const std::invalid_argument &ex)
        {
            throw_parse_exception("Malformed number (invalid argument: " + std::string{ex.what()} + ")");
        }
        catch (const std::out_of_range &ex)
        {
            throw_parse_exception("Malformed number (out of range: " + std::string{ex.what()} + ")");
        }
    }

    std::shared_ptr<value<double>> parse_float(std::string::iterator &it,
                                               const std::string::iterator &end)
    {
        std::string v{it, end};
        v.erase(std::remove(v.begin(), v.end(), '_'), v.end());
        it = end;
        char decimal_point = std::localeconv()->decimal_point[0];
        std::replace(v.begin(), v.end(), '.', decimal_point);
        try
        {
            return make_value<double>(std::stod(v));
        }
        catch (const std::invalid_argument &ex)
        {
            throw_parse_exception("Malformed number (invalid argument: " + std::string{ex.what()} + ")");
        }
        catch (const std::out_of_range &ex)
        {
            throw_parse_exception("Malformed number (out of range: " + std::string{ex.what()} + ")");
        }
    }

    std::shared_ptr<value<bool>> parse_bool(std::string::iterator &it,
                                            const std::string::iterator &end)
    {
        auto eat = consumer(it, end, [&]() {
            throw_parse_exception("attempt to parse invalid boolean value");
        });

        if (*it == 't')
        {
            eat("true");
            return make_value<bool>(true);
        }
        else if (*it == 'f')
        {
            eat("false");
            return make_value<bool>(false);
        }
        else
        {
            // should be unreachable
            throw_parse_exception("attempt to parse invalid boolean value: " +
                                  std::string{it, end});
            return nullptr;
        }
    }

    std::string::iterator find_end_of_array_element(std::string::iterator it,
                                                    std::string::iterator end)
    {
        auto ret = std::find_if(it, end, [](char c) {
            return !std::isdigit(static_cast<unsigned char>(c)) &&
                   c != '_' && c != '.' && c != 'e' &&
                   c != 'E' && c != '-' && c != '+' &&
                   c != 'x' && c != 'o' && c != 'b';
        });
        if (ret != end && ret + 1 != end && ret + 2 != end)
        {
            if ((ret[0] == 'i' && ret[1] == 'n' && ret[2] == 'f') ||
                (ret[0] == 'n' && ret[1] == 'a' && ret[2] == 'n'))
            {
                ret = ret + 3;
            }
        }
        return ret;
    }

    std::string::iterator find_end_of_number(std::string::iterator it,
                                             std::string::iterator end)
    {
        auto ret = std::find_if(it, end, [](char c) {
            return !std::isdigit(static_cast<unsigned char>(c)) &&
                   c != '_' && c != '.' && c != 'e' &&
                   c != 'E' && c != '-' && c != '+' &&
                   c != 'x' && c != 'o' && c != 'b';
        });
        if (ret != end && ret + 1 != end && ret + 2 != end)
        {
            if ((ret[0] == 'i' && ret[1] == 'n' && ret[2] == 'f') ||
                (ret[0] == 'n' && ret[1] == 'a' && ret[2] == 'n'))
            {
                ret = ret + 3;
            }
        }
        return ret;
    }

    std::string::iterator find_end_of_date(std::string::iterator it,
                                           std::string::iterator end)
    {
        auto end_of_date = std::find_if(it, end, [](char c) {
            return !std::isdigit(static_cast<unsigned char>(c)) && c != '-';
        });

        if (end_of_date != end && *end_of_date == ' ' && end_of_date + 1 != end &&
            std::isdigit(static_cast<unsigned char>(end_of_date[1])))
        {
            end_of_date++;
        }

        return std::find_if(end_of_date, end, [](char c) {
            return !std::isdigit(static_cast<unsigned char>(c)) &&
                   c != 'T' && c != 'Z' && c != ':' &&
                   c != '-' && c != '+' && c != '.';
        });
    }

    std::string::iterator find_end_of_time(std::string::iterator it,
                                           std::string::iterator end)
    {
        return std::find_if(it, end, [](char c) {
            return !std::isdigit(static_cast<unsigned char>(c)) && c != ':' && c != '.';
        });
    }

    local_time read_time(std::string::iterator &it,
                         const std::string::iterator &end)
    {
        auto time_end = find_end_of_time(it, end);

        auto eat = consumer(it, time_end, [&]() {
            throw_parse_exception("Malformed time");
        });

        local_time ltime;

        ltime.hour = eat.eat_digits(2);
        eat(':');
        ltime.minute = eat.eat_digits(2);
        eat(':');
        ltime.second = eat.eat_digits(2);

        int power = 100000;
        if (it != time_end && *it == '.')
        {
            ++it;
            while (it != time_end && std::isdigit(static_cast<unsigned char>(*it)))
            {
                ltime.nanosecond += power * (*it++ - '0') * 1000;
                power /= 10;
            }
        }

        if (it != time_end)
            throw_parse_exception("Malformed time");

        return ltime;
    }

    std::shared_ptr<value<local_time>>
    parse_time(std::string::iterator &it, const std::string::iterator &end)
    {
        return make_value(read_time(it, end));
    }

    std::shared_ptr<node> parse_date(std::string::iterator &it,
                                     const std::string::iterator &end)
    {
        auto date_end = find_end_of_date(it, end);

        auto eat = consumer(it, date_end, [&]() {
            throw_parse_exception("Malformed date");
        });

        local_date ldate;
        ldate.year = eat.eat_digits(4);
        eat('-');
        ldate.month = eat.eat_digits(2);
        eat('-');
        ldate.day = eat.eat_digits(2);

        if (it == date_end)
            return make_value(std::move(ldate));

        eat.eat_either('T', ' ');

        local_date_time ldt(std::move(ldate), read_time(it, date_end));

        if (it == date_end)
            return make_value(std::move(ldt));

        offset_date_time dt;
        static_cast<local_date_time &>(dt) = ldt;

        int hoff = 0;
        int moff = 0;
        if (*it == '+' || *it == '-')
        {
            auto plus = *it == '+';
            ++it;

            hoff = eat.eat_digits(2);
            eat(':');
            moff = eat.eat_digits(2);

            static_cast<time_offset &>(dt) = time_offset((plus) ? hoff : -hoff,
                                                         (plus) ? moff : -moff);
        }
        else if (*it == 'Z')
        {
            ++it;
        }

        if (it != date_end)
            throw_parse_exception("Malformed date");

        return make_value(std::move(dt));
    }

    std::shared_ptr<node> parse_array(std::string::iterator &it,
                                      std::string::iterator &end)
    {
        // toml v1.0.0-rc.1 removed the "homogeneity" restriction:
        // arrays can either be homogeneous, or contain mixed types

        ++it;
        skip_whitespace_and_comments(it, end);

        auto arr = make_array();
        while (it != end && *it != ']')
        {
            skip_whitespace_and_comments(it, end);
            arr->push_back(parse_value(it, end));
            skip_whitespace_and_comments(it, end);
            if (*it != ',')
                break;
            ++it;
            skip_whitespace_and_comments(it, end);
        }

        if (it == end)
        {
            throw_parse_exception("missing closing `]` in array");
        }
        else
        {
            ++it;
            return arr;
        }
    }

    std::shared_ptr<table> parse_inline_table(std::string::iterator &it,
                                              std::string::iterator &end)
    {
        auto tbl = make_table(true);
        do
        {
            ++it;
            if (it == end)
                throw_parse_exception("Unterminated inline table");

            consume_whitespace(it, end);
            if (it != end && *it != '}')
            {
                parse_key_value(it, end, tbl.get());
                consume_whitespace(it, end);
            }
        } while (*it == ',');

        if (it == end || *it != '}')
            throw_parse_exception("Unterminated inline table");

        ++it;
        consume_whitespace(it, end);

        return tbl;
    }

    void skip_whitespace_and_comments(std::string::iterator &start,
                                      std::string::iterator &end)
    {
        consume_whitespace(start, end);
        while (start == end || *start == '#')
        {
            if (!detail::getline(input_, line_))
                throw_parse_exception("Unclosed array");
            line_number_++;
            start = line_.begin();
            end = line_.end();
            consume_whitespace(start, end);
        }
    }

    void consume_whitespace(std::string::iterator &it,
                            const std::string::iterator &end)
    {
        while (it != end && (*it == ' ' || *it == '\t'))
            ++it;
    }

    void consume_backwards_whitespace(std::string::iterator &back,
                                      const std::string::iterator &front)
    {
        while (back != front && (*back == ' ' || *back == '\t'))
            --back;
    }

    void eol_or_comment(const std::string::iterator &it,
                        const std::string::iterator &end)
    {
        if (it != end && *it != '#')
            throw_parse_exception("Unidentified trailing character '" + std::string{*it} + "'---did you forget a '#'?");
    }

    bool is_time(const std::string::iterator &it,
                 const std::string::iterator &end)
    {
        auto time_end = find_end_of_time(it, end);
        auto len = std::distance(it, time_end);

        if (len < 8)
            return false;

        if (it[2] != ':' || it[5] != ':')
            return false;

        if (len > 8)
            return it[8] == '.' && len > 9;

        return true;
    }

    numeric_type determine_date_type(const std::string::iterator &it,
                                     const std::string::iterator &end)
    {
        auto date_end = find_end_of_date(it, end);
        auto len = std::distance(it, date_end);

        if (len < 10)
            return numeric_type::None;
        ;

        if (it[4] != '-' || it[7] != '-')
            return numeric_type::None;

        if (len >= 19 && (it[10] == 'T' || it[10] == ' ') && is_time(it + 11, date_end))
        {
            // datetime type
            auto time_end = find_end_of_time(it + 11, date_end);
            if (time_end == date_end)
                return {numeric_type::LocalDateTime};
            else
                return {numeric_type::OffsetDateTime};
        }
        else if (len == 10)
        {
            // just a regular date
            return {numeric_type::LocalDate};
        }

        return {};
    }

    std::istream &input_;
    std::string line_;
    std::size_t line_number_ = 0;
};

parse_result parse_file(const std::string &file_path)
{
    std::ifstream file{file_path};

    if (!file.is_open())
    {
        return {parse_error(file_path + " could not be opened for parsing")};
    }
    else
    {
        try
        {
            parser p{file};
            return {p.parse()};
        }
        catch (const parse_error &e)
        {
            return {std::move(e)};
        }
    }
}

parse_result parse(const std::string &source)
{
    try
    {
        std::stringstream source_stream{source};
        parser p{source_stream};
        return {p.parse()};
    }
    catch (const parse_error &e)
    {
        return {e};
    }
}
TOML_NAMESPACE_END
} // namespace toml