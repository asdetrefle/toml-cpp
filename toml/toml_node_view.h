#pragma once

#include "toml_node.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

template <typename T>
class node_view final
{
public:
    node_view() noexcept = default;

    node_view(std::shared_ptr<T> &&node) noexcept
        : node_{std::const_pointer_cast<std::remove_cv_t<T>>(node)} {}

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

    template <typename U>
    bool is_value() const noexcept
    {
        return node_ && node_->template is_value<U>();
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

    template <class U>
    auto as_value()
    {
        return node_ ? node_->template as_value<U>() : nullptr;
    }

    template <class U>
    auto as_value() const
    {
        return node_ ? node_->template as_value<U>() : nullptr;
    }

    auto as_array()
    {
        return node_ ? node_->as_array() : nullptr;
    }

    auto as_table()
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

    template <typename U>
    auto value_or(U &&default_value) const noexcept
    {
        using return_type = decltype(node_->value_or(std::forward<U>(default_value)));
        if (node_)
        {
            return node_->value_or(std::forward<U>(default_value));
        }
        else
        {
            return return_type{std::forward<U>(default_value)};
        }
    }

    /*
    /// \brief	Invokes a visitor on the viewed node based on its concrete type.
    ///
    /// \remarks Has no effect if the view does not reference a node.
    ///
    /// \see node::visit()
    template <typename FUNC>
    decltype(auto) visit(FUNC &&visitor) const
        TOML_MAY_THROW_UNLESS(visit_is_nothrow<FUNC &&>)
    {
        using return_type = decltype(node_->visit(std::forward<FUNC>(visitor)));
        if (node_)
            return node_->visit(std::forward<FUNC>(visitor));
        if constexpr (!std::is_void_v<return_type>)
            return return_type{};
    }

    /// \brief	Gets a raw reference to the viewed node's underlying data.
    ///
    /// \warning This function is dangerous if used carelessly and **WILL** break your code if the
    /// 		 node_view didn't reference a node, or the chosen value type doesn't match the node's
    /// 		 actual type. If you provide a definition for TOML_ASSERT (explicitly or indirectly
    /// 		 by including `<cassert>`) an assertion will fire when invalid accesses are attempted: \cpp
    ///
    /// auto tbl = toml::parse(R"(
    ///
    ///	min = 32
    ///	max = 45
    ///
    /// )"sv);
    ///
    /// auto& min_ref = tbl["min"].ref<int64_t>(); // this is OK
    /// auto& max_ref = tbl["max"].ref<double>(); // hits assertion because the type is wrong
    /// auto& min_ref = tbl["foo"].ref<int64_t>(); // hits assertion because "foo" didn't exist
    ///
    /// \ecpp
    ///
    /// \tparam	U	One of the TOML value types.
    ///
    /// \returns	A reference to the underlying data.
    template <typename U>
    decltype(auto) ref() const noexcept
    {
        using type = impl::unwrapped<U>;
        static_assert(
            impl::is_value_or_node<type>,
            "Template type parameter must be one of the TOML value types, a toml::table, or a toml::array");
        TOML_ASSERT(
            node_ && "toml::node_view::ref() called on a node_view that did not reference a node");
        return node_->template ref<type>();
    }

    /// \brief	Returns true if the viewed node is a table with the same contents as RHS.
    friend bool operator==(const node_view &lhs, const table &rhs) noexcept
    {
        if (lhs.node_ == &rhs)
            return true;
        const auto tbl = lhs.as<table>();
        return tbl && *tbl == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const node_view &, const table &, )

    /// \brief	Returns true if the viewed node is an array with the same contents as RHS.
    friend bool operator==(const node_view &lhs, const array &rhs) noexcept
    {
        if (lhs.node_ == &rhs)
            return true;
        const auto arr = lhs.as<array>();
        return arr && *arr == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const node_view &, const array &, )

    /// \brief	Returns true if the viewed node is a value with the same value as RHS.
    template <typename U>
    friend bool operator==(const node_view &lhs, const toml::value<U> &rhs) noexcept
    {
        if (lhs.node_ == &rhs)
            return true;
        const auto val = lhs.as<U>();
        return val && *val == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const node_view &, const toml::value<U> &, template <typename U>)

    /// \brief	Returns true if the viewed node is a value with the same value as RHS.
    template <typename U, typename = std::enable_if_t<impl::is_value_or_promotable<U>>>
    friend bool operator==(const node_view &lhs, const U &rhs) noexcept
    {
        const auto val = lhs.as<impl::promoted<U>>();
        return val && *val == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(
        const node_view &,
        const U &,
        template <typename U, typename = std::enable_if_t<impl::is_value_or_promotable<U>>>
    )

    /// \brief	Returns true if the viewed node is an array with the same contents as the RHS initializer list.
    template <typename U>
    friend bool operator==(const node_view &lhs, const std::initializer_list<U> &rhs) noexcept
    {
        const auto arr = lhs.as<array>();
        return arr && *arr == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const node_view &, const std::initializer_list<U> &, template <typename U>)

    /// \brief	Returns true if the viewed node is an array with the same contents as the RHS vector.
    template <typename U>
    friend bool operator==(const node_view &lhs, const std::vector<U> &rhs) noexcept
    {
        const auto arr = lhs.as<array>();
        return arr && *arr == rhs;
    }
    TOML_ASYMMETRICAL_EQUALITY_OPS(const node_view &, const std::vector<U> &, template <typename U>)
    */

    node_view<T> operator[](std::string_view key)
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

    node_view<T> operator[](size_t index)
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

private:
    std::shared_ptr<std::remove_cv_t<T>> node_;
};

template class node_view<node>;
template class node_view<const node>;
TOML_NAMESPACE_END
} // namespace toml