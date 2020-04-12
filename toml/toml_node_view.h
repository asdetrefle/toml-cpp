#pragma once

#include "toml_node.h"
#include "toml_value.h"
#include "toml_array.h"
#include "toml_table.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

class node_view final
{
public:
    node_view() noexcept = default;

    node_view(std::shared_ptr<node> &&node) noexcept
        : node_{node} {}

    explicit operator bool() const noexcept
    {
        return node_ != nullptr;
    }

    base_type type() const noexcept
    {
        return node_ ? node_->type() : base_type::None;
    }

    bool is_value() const noexcept
    {
        return node_ && node_->is_value();
    }

    template <typename T>
    bool is_value() const noexcept
    {
        return node_ && node_->template is_value<T>();
    }

    bool is_table() const noexcept
    {
        return node_ && node_->is_table();
    };

    bool is_array() const noexcept
    {
        return node_ && node_->is_array();
    }

    bool is_table_array() const noexcept
    {
        return node_ && node_->is_table_array();
    }

    template <class T>
    auto as_value() const
    {
        return node_ ? node_->template as_value<T>() : nullptr;
    }

    auto as_array() const
    {
        return node_ ? node_->as_array() : nullptr;
    }

    auto as_table() const
    {
        return node_ ? node_->as_table() : nullptr;
    }

    template <typename U>
    std::optional<U> value() const noexcept
    {
        if (node_)
        {
            return node_->template value<U>();
        }
        else
        {
            return {};
        }
    }

    template <typename T>
    auto value_or(T &&default_value) const noexcept
    {
        using return_type = decltype(node_->value_or(std::forward<T>(default_value)));
        if (node_)
        {
            return node_->value_or(std::forward<T>(default_value));
        }
        else
        {
            return return_type{std::forward<T>(default_value)};
        }
    }

    node_view operator[](std::string_view key) const
    {
        if (auto tbl = this->as_table())
        {
            return {(*tbl)[key]};
        }
        else
        {
            return {nullptr};
        }
    }

    node_view operator[](size_t index) const
    {
        if (auto arr = this->as_array())
        {
            return {(*arr)[index]};
        }
        else
        {
            return {nullptr};
        }
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>>
    std::optional<U> map(F f) const
    {
        if (auto val = this->value<T>())
        {
            return {f(val.value())};
        }
        else
        {
            return std::nullopt;
        }
    }

    template <typename T>
    std::vector<T> collect() const
    {
        if (auto arr = this->as_array())
        {
            return arr->template collect<T>();
        }
        else
        {
            return {};
        }
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>>
    std::vector<U> map_collect(F f) const
    {
        if (auto arr = this->as_array())
        {
            return arr->template map_collect<T>(f);
        }
        else
        {
            return {};
        }
    }

    /*
    /// \brief	Invokes a visitor on the viewed node based on its concrete type.
    ///
    /// \remarks Has no effect if the view does not reference a node.
    ///
    /// \see node::visit()
    template <typename FUNC>
    decltype(auto) visit(FUNC &&visitor) const TOML_MAY_THROW_UNLESS(visit_is_nothrow<FUNC &&>)
    {
        using return_type = decltype(node_->visit(std::forward<FUNC>(visitor)));
        if (node_)
            return node_->visit(std::forward<FUNC>(visitor));
        if constexpr (!std::is_void_v<return_type>)
            return return_type{};
    }
    */

private:
    std::shared_ptr<node> node_;
};

TOML_NAMESPACE_END
} // namespace toml