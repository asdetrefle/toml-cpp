#pragma once

#include <assert.h>
#include "toml_base.h"

namespace toml
{
TOML_NAMESPACE_BEGIN
/**
 * A generic base TOML value used for type erasure.
 */
class node : public std::enable_shared_from_this<node>
{
    friend class parser;

public:
    virtual std::shared_ptr<node> clone() const = 0;

    virtual ~node() noexcept = default;

    node_view view() const noexcept;

    virtual base_type type() const noexcept
    {
        return type_;
    }

    bool is_value() const noexcept
    {
        return static_cast<uint8_t>(type_) > 0 &&
               static_cast<uint8_t>(type_) < 9;
    }

    template <typename T>
    bool is_value() const noexcept
    {
        return is_value() && type_ == value_type_traits<T>::value;
    }

    bool is_table() const noexcept
    {
        return type_ == base_type::Table;
    };

    bool is_array() const noexcept
    {
        return type_ == base_type::Array || type_ == base_type::TableArray;
    }

    virtual bool is_table_array() const noexcept
    {
        return false;
    }

    template <class T>
    std::shared_ptr<toml::value<T>> as_value()
    {
        if (type() == base_type_traits<T>::value)
        {
            return std::static_pointer_cast<toml::value<T>>(shared_from_this());
        }
        else
        {
            return nullptr;
        }
    }

    template <class T>
    std::shared_ptr<const toml::value<T>> as_value() const
    {
        if (type() == base_type_traits<T>::value)
        {
            return std::static_pointer_cast<const toml::value<T>>(shared_from_this());
        }
        else
        {
            return nullptr;
        }
    }

    std::shared_ptr<array> as_array()
    {
        if (is_array())
        {
            return std::static_pointer_cast<toml::array>(shared_from_this());
        }
        else
        {
            return nullptr;
        }
    }

    std::shared_ptr<const array> as_array() const
    {
        if (is_array())
        {
            return std::static_pointer_cast<const toml::array>(shared_from_this());
        }
        else
        {
            return nullptr;
        }
    }

    std::shared_ptr<table> as_table()
    {
        if (is_table())
        {
            return std::static_pointer_cast<toml::table>(shared_from_this());
        }
        else
        {
            return nullptr;
        }
    }

    std::shared_ptr<const table> as_table() const
    {
        if (is_table())
        {
            return std::static_pointer_cast<const toml::table>(shared_from_this());
        }
        else
        {
            return nullptr;
        }
    }

    template <typename T>
    inline std::optional<T> value() const noexcept; // implemented in toml_value.h

    template <typename T, typename U = typename value_type_traits<std::decay_t<T>>::type>
    inline auto value_or(T &&default_value) const noexcept
    {
        static_assert(is_value_promotable<std::decay_t<T>>,
                      "default value must be of (or be promotable to) one of the TOML types");

        if (auto val = this->template value<U>())
        {
            return *val;
        }
        else
        {
            return U{std::forward<T>(default_value)};
        }
    }

    template <typename T, typename = std::enable_if_t<std::is_nothrow_default_constructible_v<T>>>
    inline auto value_or_default() const noexcept
    {
        return this->template value_or<T>({});
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<!std::is_void_v<U>>>
    std::optional<U> map(F &&f) const
    {
        if constexpr (is_value_promotable<T>)
        {
            if (const auto val = this->template value<T>())
            {
                return {f(val.value())};
            }
        }
        else if constexpr (std::is_same_v<T, array>)
        {
            if (const auto arr = this->as_array())
            {
                return {f(*arr)};
            }
        }
        else if constexpr (std::is_same_v<T, table>)
        {
            if (const auto tbl = this->as_table())
            {
                return {f(*tbl)};
            }
        }
        else if constexpr (std::is_same_v<T, node_view>)
        {
            return {f(this->view())};
        }
        return std::nullopt;
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<std::is_void_v<U>>>
    void map(F &&f) const
    {
        if constexpr (is_value_promotable<T>)
        {
            if (const auto val = this->template value<T>())
            {
                f(val.value());
            }
        }
        else if constexpr (std::is_same_v<T, array>)
        {
            if (const auto arr = this->as_array())
            {
                f(*arr);
            }
        }
        else if constexpr (std::is_same_v<T, table>)
        {
            if (const auto tbl = this->as_table())
            {
                f(*tbl);
            }
        }
    }

    template <class Visitor, class... Args>
    void accept(Visitor &&visitor, Args &&... args) const;

protected:
    node() noexcept = default;

    node(base_type t)
        : type_(t) {}

private:
    base_type type_{base_type::None};
};

template <class... Ts>
struct value_accept;

template <>
struct value_accept<>
{
    template <class Visitor, class... Args>
    static void accept(const node &, Visitor &&, Args &&...) {}
};

template <class T, class... Ts>
struct value_accept<T, Ts...>
{
    template <class Visitor, class... Args>
    static void accept(const node &b, Visitor &&visitor, Args &&... args)
    {
        if (auto v = b.as_value<T>())
        {
            visitor.visit(*v, std::forward<Args>(args)...);
        }
        else
        {
            value_accept<Ts...>::accept(b, std::forward<Visitor>(visitor),
                                        std::forward<Args>(args)...);
        }
    }
};

template <class Visitor, class... Args>
void node::accept(Visitor &&visitor, Args &&... args) const
{
    if (is_value())
    {
        using value_acceptor = value_accept<std::string, int64_t, double, bool, local_date,
                                            local_time, local_date_time, offset_date_time>;

        value_acceptor::accept(*this, std::forward<Visitor>(visitor),
                               std::forward<Args>(args)...);
    }
    else if (is_table())
    {
        visitor.visit(static_cast<const toml::table &>(*as_table()),
                      std::forward<Args>(args)...);
    }
    else if (is_array())
    {
        visitor.visit(static_cast<const toml::array &>(*as_array()),
                      std::forward<Args>(args)...);
    }
}

TOML_NAMESPACE_END
} // namespace toml