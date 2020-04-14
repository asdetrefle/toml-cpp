#pragma once

#include "toml_node.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

/**
 * A concrete TOML value representing the "leaves" of the "tree".
 */
template <class T>
class value final : public node
{
    struct make_shared_enabler
    {
    };

    template <class U>
    friend std::shared_ptr<value<typename value_type_traits<U>::type>> make_value(U &&val);

    static_assert(std::is_same_v<T, typename base_type_traits<T>::type>,
                  "Template type parameter must be one of the TOML value types");

public:
    std::shared_ptr<node> clone() const override
    {
        // just make a copy of original data_
        return make_value(T(data_));
    }

    value(const make_shared_enabler &, const T &val)
        : value(val) {}

    T &get()
    {
        return data_;
    }

    const T &get() const
    {
        return data_;
    }

private:
    T data_;

    value(const T &val)
        : node(base_type_traits<T>::value),
          data_(val) {}

    value(const value &val) = delete;
    value &operator=(const value &val) = delete;
};

template <class T>
std::shared_ptr<value<typename value_type_traits<T>::type>> make_value(T &&val)
{
    static_assert(is_value_promotable<T>,
                  "make_value type must be of (or be promotable to) one of the TOML types");
    using value_type = value<typename value_type_traits<T>::type>;
    using enabler = typename value_type::make_shared_enabler;
    return std::make_shared<value_type>(enabler{}, std::forward<T>(val));
}

TOML_NAMESPACE_END
} // namespace toml