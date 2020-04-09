#pragma once

#include <toml_base.h>

namespace toml
{
TOML_NAMESPACE_BEGIN
/**
 * A generic base TOML value used for type erasure.
 */
class node : public std::enable_shared_from_this<node>
{
public:
    virtual std::shared_ptr<node> clone() const = 0;

    virtual ~node() noexcept = default;

    virtual base_type type() const noexcept = 0;

    virtual bool is_value() const noexcept
    {
        return false;
    }

    virtual bool is_table() const noexcept
    {
        return false;
    };

    virtual bool is_array() const noexcept
    {
        return false;
    }
    virtual bool is_table_array() const
    {
        return false;
    }

    /**
     * Converts the TOML element into a table.
     */
    std::shared_ptr<table> as_table()
    {
        if (is_table())
            return std::static_pointer_cast<table>(shared_from_this());
        return nullptr;
    }

    /**
     * Converts the TOML element to an array.
     */
    std::shared_ptr<array> as_array()
    {
        if (is_array())
            return std::static_pointer_cast<array>(shared_from_this());
        return nullptr;
    }

    /**
     * Determines if the given TOML element is an array of tables.
     */

    /**
     * Converts the TOML element into a table array.
     */
    std::shared_ptr<table_array> as_table_array()
    {
        if (is_table_array())
            return std::static_pointer_cast<table_array>(shared_from_this());
        return nullptr;
    }

    template <typename T>
    [[nodiscard]] auto value_or(T &&default_value) const noexcept;
    /**
     * Attempts to coerce the TOML element into a concrete TOML value
     * of type T.
     */
    template <class T>
    std::shared_ptr<value<T>> as();

    template <class T>
    std::shared_ptr<const value<T>> as() const;

    template <class Visitor, class... Args>
    void accept(Visitor &&visitor, Args &&... args) const;

    template <typename T>
    inline std::optional<T> value() const noexcept
    {
        static_assert(is_value<T> || std::is_same_v<T, string_view>,
                      "Value type must be one of the TOML value types (or string_view)");

        switch (type())
        {
        case base_type::None:
            [[fallthrough]];
        case base_type::Table:
            [[fallthrough]];
        case base_type::Array:
        {
            return {};
        }
        case base_type::String:
        {
            if constexpr (std::is_same_v<T, string> || std::is_same_v<T, string_view>)
                return {T{ref_cast<string>().get()}};
            else
                return {};
        }
        case base_type::Integer:
        {
            if constexpr (std::is_same_v<T, int64_t>)
                return ref_cast<int64_t>().get();
            else if constexpr (std::is_same_v<T, double>)
                return static_cast<double>(ref_cast<int64_t>().get());
            else
                return {};
        }
        case base_type::Float:
        {
            if constexpr (std::is_same_v<T, double>)
                return ref_cast<double>().get();
            else if constexpr (std::is_same_v<T, int64_t>)
                return static_cast<int64_t>(ref_cast<double>().get());
            else
                return {};
        }
        case base_type::Boolean:
        {
            if constexpr (std::is_same_v<T, bool>)
                return ref_cast<bool>().get();
            else
                return {};
        }
        case base_type::LocalDate:
        {
            if constexpr (std::is_same_v<T, LocalDate>)
                return ref_cast<LocalDate>().get();
            else
                return {};
        }
        case base_type::LocalTime:
        {
            if constexpr (std::is_same_v<T, LocalTime>)
                return ref_cast<LocalTime>().get();
            else
                return {};
        }
        case base_type::LocalDateTime:
        {
            if constexpr (std::is_same_v<T, LocalDateTime>)
                return ref_cast<LocalDateTime>().get();
            else
                return {};
        }
        case base_type::OffsetDateTime:
        {
            if constexpr (std::is_same_v<T, OffsetDateTime>)
                return ref_cast<LocalDateTime>().get();
            else
                return {};
        }
        default:
            return {}
        }
    }

    template <typename T>
    inline auto value_or(T &&default_value) const noexcept
    {
        static_assert(is_value_promotable<remove_cvref_t<T>>,
                      "Default value type must be (or be promotable to) one of the TOML value types");

        using toml_type = value_traits<remove_cvref_t<T>>::type;

        using return_type = std::conditional_t<
            std::is_same_v<toml_type, string>,
            std::conditional_t<std::is_same_v<impl::remove_cvref_t<T>, string>, string, string_view>,
            toml_type>;

        if (auto val = this->value<return_type>())
        {
            return *val;
        }
        else
        {
            return_type{std::forward<T>(default_value)};
        }
    }

    base_type type() const
    {
        return type_;
    }

protected:
    node(const base_type t)
        : type_(t) {}

private:
    const base_type type_{base_type::None};
};

template <typename T>
class node_view final
{
public:
    explicit operator bool() const noexcept { return node_ != nullptr; }
    viewed_type *get() const noexcept { return node_; }

    /// \brief	Returns the type identifier for the viewed node.
    node_type type() const noexcept { return node_ ? node_->type() : node_type::none; }

    bool is_table() const noexcept { return node_ && node_->is_table(); }
    bool is_array() const noexcept { return node_ && node_->is_array(); }
    bool is_value() const noexcept { return node_ && node_->is_value(); }
    bool is_string() const noexcept { return node_ && node_->is_string(); }
    bool is_integer() const noexcept { return node_ && node_->is_integer(); }
    bool is_floating_point() const noexcept { return node_ && node_->is_floating_point(); }
    bool is_number() const noexcept { return node_ && node_->is_number(); }
    bool is_boolean() const noexcept { return node_ && node_->is_boolean(); }
    bool is_date() const noexcept { return node_ && node_->is_date(); }
    bool is_time() const noexcept { return node_ && node_->is_time(); }
    bool is_date_time() const noexcept { return node_ && node_->is_date_time(); }
    bool is_array_of_tables() const noexcept { return node_ && node_->is_array_of_tables(); }

    /// \brief	Checks if this view references a node of a specific type.
    ///
    /// \tparam	U	A TOML node or value type.
    ///
    /// \returns	Returns true if the viewed node is an instance of the specified type.
    ///
    /// \see toml::node::is()
    template <typename U>
    bool is() const noexcept
    {
        return node_ ? node_->template is<U>() : false;
    }

    template <typename U>
    auto as() const noexcept
    {
        static_assert(
            impl::is_value_or_node<impl::unwrapped<U>>,
            "Template type parameter must be one of the basic value types, a toml::table, or a toml::array");

        return node_ ? node_->template as<U>() : nullptr;
    }

    auto as_table() const noexcept { return as<table>(); }
    auto as_array() const noexcept { return as<array>(); }
    auto as_string() const noexcept { return as<string>(); }
    auto as_integer() const noexcept { return as<int64_t>(); }
    auto as_floating_point() const noexcept { return as<double>(); }
    auto as_boolean() const noexcept { return as<bool>(); }
    auto as_date() const noexcept { return as<date>(); }
    auto as_time() const noexcept { return as<time>(); }
    auto as_date_time() const noexcept { return as<date_time>(); }

    /// \brief	Gets the raw value contained by the referenced node.
    ///
    /// \tparam	U	One of the TOML value types. Can also be a string_view.
    ///
    /// \returns	The underlying value if the node was a value of the matching type (or convertible to it), or an empty optional.
    ///
    /// \see node::value()
    template <typename U>
    optional<U> value() const noexcept
    {
        if (node_)
            return node_->template value<U>();
        return {};
    }

    /// \brief	Gets the raw value contained by the referenced node, or a default.
    ///
    /// \tparam	U	Default value type. Must be (or be promotable to) one of the TOML value types.
    /// \param 	default_value	The default value to return if the view did not reference a node,
    /// 						or if the node wasn't a value, wasn't the correct type, or no conversion was possible.
    ///
    /// \returns	The node's underlying value, or the default if the node wasn't a value, wasn't the
    /// 			correct type, or no conversion was possible.
    ///
    /// \see node::value_or()
    template <typename U>
    auto value_or(U &&default_value) const noexcept
    {
        using return_type = decltype(node_->value_or(std::forward<U>(default_value)));
        if (node_)
            return node_->value_or(std::forward<U>(default_value));
        return return_type{std::forward<U>(default_value)};
    }

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

    /// \brief	Returns a view of the selected subnode.
    ///
    /// \param 	key	The key of the node to retrieve
    ///
    /// \returns	A view of the selected node if this node represented a table and it contained a
    /// 			value at the given key, or an empty view.
    node_view operator[](string_view key) const noexcept
    {
        if (auto tbl = this->as_table())
            return {tbl->get(key)};
        return {nullptr};
    }

    /// \brief	Returns a view of the selected subnode.
    ///
    /// \param 	index The index of the node to retrieve
    ///
    /// \returns	A view of the selected node if this node represented an array and it contained a
    /// 			value at the given index, or an empty view.
    node_view operator[](size_t index) const noexcept
    {
        if (auto arr = this->as_array())
            return {arr->get(index)};
        return {nullptr};
    }

private:
    friend class toml::table;
    template <typename U>
    friend class toml::node_view;

    mutable T *node_;

    TOML_NODISCARD_CTOR
    node_view(viewed_type *node) noexcept
        : node_{node}
    {
    }

    template <typename FUNC>
    static constexpr bool visit_is_nothrow = noexcept(std::declval<viewed_type *>()->visit(std::declval<FUNC &&>()));

    template <typename CHAR, typename U>
    friend std::basic_ostream<CHAR> &operator<<(std::basic_ostream<CHAR> &, const node_view<U> &) TOML_MAY_THROW;
};

/// \brief	Prints the viewed node out to a stream.
template <typename CHAR, typename T>
inline std::basic_ostream<CHAR> &operator<<(std::basic_ostream<CHAR> &os, const node_view<T> &nv) TOML_MAY_THROW
{
    if (nv.node_)
    {
        nv.node_->visit([&os](const auto &n) TOML_MAY_THROW {
            os << n;
        });
    }
    return os;
}

template class node_view<node>;
template class node_view<const node>;
TOML_NAMESPACE_END
} // namespace toml