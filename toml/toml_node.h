#pragma once

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
    inline std::optional<T> value() const noexcept
    {
        static_assert(toml::is_value_promotable<T>,
                      "value type must be one of the TOML value types (or string_view)");

        switch (type())
        {
        case base_type::None:
            [[fallthrough]];
        case base_type::Table:
            [[fallthrough]];
        case base_type::Array:
        {
            return std::nullopt;
        }
        case base_type::String:
        {
            if constexpr (value_type_traits<T>::value == base_type::String)
                return {T{as_value<std::string>()->get()}};
            else
                return std::nullopt;
        }
        case base_type::Integer:
        {
            if constexpr (value_type_traits<T>::value == base_type::Integer ||
                          value_type_traits<T>::value == base_type::Float)
            {
                return {static_cast<T>(as_value<int64_t>()->get())};
            }
            else
            {
                return std::nullopt;
            }
        }
        case base_type::Float:
        {
            if constexpr (value_type_traits<T>::value == base_type::Float)
            {
                return {static_cast<T>(as_value<double>()->get())};
            }
            else
            {
                return std::nullopt;
            }
        }
        case base_type::Boolean:
        {
            if constexpr (value_type_traits<T>::value == base_type::Boolean)
            {
                return {as_value<bool>()->get()};
            }
            else
            {
                return std::nullopt;
            }
        }
        case base_type::LocalDate:
        {
            if constexpr (value_type_traits<T>::value == base_type::LocalDate)
            {
                return {as_value<local_date>()->get()};
            }
            else
            {
                return std::nullopt;
            }
        }
        case base_type::LocalTime:
        {
            if constexpr (value_type_traits<T>::value == base_type::LocalTime)
            {
                return {as_value<local_time>()->get()};
            }
            else
            {
                return std::nullopt;
            }
        }
        case base_type::LocalDateTime:
        {
            if constexpr (value_type_traits<T>::value == base_type::LocalDateTime)
            {
                return {as_value<local_date_time>()->get()};
            }
            else
            {
                return std::nullopt;
            }
        }
        case base_type::OffsetDateTime:
        {
            if constexpr (value_type_traits<T>::value == base_type::OffsetDateTime)
            {
                return {as_value<offset_date_time>()->get()};
            }
            else
            {
                return std::nullopt;
            }
        }
        default:
            return std::nullopt;
        }
    }

    template <typename T>
    inline auto value_or(T &&default_value) const noexcept
    {
        static_assert(is_value_promotable<std::decay_t<T>>,
                      "default value must be of (or be promotable to) one of the TOML types");

        using return_type = typename value_type_traits<std::decay_t<T>>::type;
        if (auto val = this->value<return_type>())
        {
            return *val;
        }
        else
        {
            return return_type{std::forward<T>(default_value)};
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