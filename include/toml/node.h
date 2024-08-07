#pragma once

#include <assert.h>

#include "base.h"

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

    inline node_view view() const noexcept;

    virtual base_type type() const noexcept
    {
        return type_;
    }

    template <typename T>
    bool is() const noexcept
    {
        if constexpr (std::is_same_v<T, array>)
        {
            return type_ == base_type::Array || type_ == base_type::TableArray;
        }
        else
        {
            return type_ == value_type_traits<T>::value;
        }
    }

    bool is_value() const noexcept
    {
        return static_cast<uint8_t>(type_) > 0 && static_cast<uint8_t>(type_) < 9;
    }

    bool is_table() const noexcept
    {
        return this->is<table>();
    };

    bool is_array() const noexcept
    {
        return this->is<array>();
    }

    virtual bool is_table_array() const noexcept
    {
        return false;
    }

    template <class T>
    std::shared_ptr<toml::value<T>> as_value()
    {
        return type() == base_type_traits<T>::value
                   ? std::static_pointer_cast<toml::value<T>>(shared_from_this())
                   : nullptr;
    }

    template <class T>
    std::shared_ptr<const toml::value<T>> as_value() const
    {
        return type() == base_type_traits<T>::value
                   ? std::static_pointer_cast<const toml::value<T>>(shared_from_this())
                   : nullptr;
    }

    template <typename T, typename = std::enable_if_t<value_type_traits<std::decay_t<T>>::value != base_type::None>>
    inline auto as()
    {
        if constexpr (is_one_of_v<std::remove_cv_t<T>, array, table>)
        {
            if (!is<array>() && !is<table>())
            {
                throw std::runtime_error("cannot convert toml::node to array or table");
            }
            return std::static_pointer_cast<std::remove_cv_t<T>>(shared_from_this());
        }
        else
        {
            return value<typename value_type_traits<std::decay_t<T>>::type>().value();
        }
    }

    template <typename T, typename U = typename value_type_traits<std::decay_t<T>>::type>
    inline auto as(T &&default_value) const noexcept
    {
        static_assert(is_value_promotable<std::decay_t<T>>,
                      "default value must be of (or be promotable to) one of the TOML types");

        if (auto val = this->template value<U>())
        {
            return *val;
        }
        return U{std::forward<T>(default_value)};
    }

    template <typename T, typename = std::enable_if_t<value_type_traits<std::decay_t<T>>::value != base_type::None>>
    inline auto get() const
    {
        if constexpr (is_one_of_v<std::remove_cv_t<T>, array, table>)
        {
            return is<array>() || is<table>()
                       ? std::static_pointer_cast<std::add_const_t<T>>(shared_from_this())
                       : std::shared_ptr<std::add_const_t<T>>();
        }
        else
        {
            return value<typename value_type_traits<std::decay_t<T>>::type>();
        }
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<!std::is_void_v<U>>>
    std::optional<U> map(F &&f) const
    {
        if constexpr (std::is_same_v<T, node_view>)
        {
            return {f(this->view())};
        }
        else
        {
            if (const auto val = this->template get<T>())
            {
                return {f(*val)};
            }
            else
            {
                return std::nullopt;
            }
        }
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<std::is_void_v<U>>>
    void map(F &&f) const
    {
        if constexpr (std::is_same_v<T, node_view>)
        {
            f(this->view());
        }
        else
        {
            if (const auto val = this->template as<T>())
            {
                f(val.value());
            }
        }
    }

    template <class Visitor, class... Args>
    void accept(Visitor &&visitor, Args &&...args) const;

protected:
    node() noexcept = default;

    node(base_type t)
        : type_(t) {}

private:
    base_type type_{base_type::None};

    template <typename T>
    inline std::optional<T> value() const noexcept; // implemented in toml_value.h
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
    static void accept(const node &b, Visitor &&visitor, Args &&...args)
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
void node::accept(Visitor &&visitor, Args &&...args) const
{
    if (is_value())
    {
        using value_acceptor = value_accept<std::string, int64_t, double, bool, local_date,
                                            local_time, local_date_time, offset_date_time>;

        value_acceptor::accept(*this, std::forward<Visitor>(visitor),
                               std::forward<Args>(args)...);
    }
    else if (is<table>())
    {
        visitor.visit(static_cast<const toml::table &>(*get<table>()),
                      std::forward<Args>(args)...);
    }
    else if (is<array>())
    {
        visitor.visit(static_cast<const toml::array &>(*get<array>()),
                      std::forward<Args>(args)...);
    }
}

TOML_NAMESPACE_END
} // namespace toml
