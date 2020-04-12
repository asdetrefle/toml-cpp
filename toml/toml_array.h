#pragma once

#include <vector>

#include "toml_base.h"
#include "toml_node.h"

namespace toml
{
TOML_NAMESPACE_BEGIN

class array final : public node
{
    struct make_shared_enabler
    {
    };
    friend std::shared_ptr<array> make_array();

public:
    using value_type = std::shared_ptr<node>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    /// \brief A RandomAccessIterator for iterating over nodes in a toml::array.
    using iterator = std::vector<std::shared_ptr<node>>::iterator;
    /// \brief A RandomAccessIterator for iterating over const nodes in a toml::array.
    using const_iterator = std::vector<std::shared_ptr<node>>::const_iterator;

    array(const make_shared_enabler &) noexcept
        : node(base_type::Array) {}

    std::shared_ptr<node> clone() const override
    {
        auto result = make_array();
        result->reserve(nodes_.size());
        for (const auto &ptr : nodes_)
        {
            result->push_back(ptr->clone());
        }
        return result;
    }

    base_type type() const noexcept
    {
        return is_table_array() ? base_type::TableArray : node::type();
    }

    iterator begin() noexcept
    {
        return nodes_.begin();
    }

    const_iterator begin() const noexcept
    {
        return nodes_.begin();
    }

    const_iterator cbegin() const noexcept
    {
        return nodes_.cbegin();
    }

    iterator end() noexcept
    {
        return nodes_.end();
    }

    const_iterator end() const noexcept
    {
        return nodes_.end();
    }

    const_iterator cend() const noexcept
    {
        return nodes_.cend();
    }

    bool empty() const noexcept
    {
        return nodes_.empty();
    }

    size_t size() const noexcept
    {
        return nodes_.size();
    }

    void reserve(size_type n)
    {
        nodes_.reserve(n);
    }

    template <typename T>
    bool is_homogeneous() const noexcept
    {
        if (nodes_.empty())
        {
            return false;
        }

        for (const auto &n : nodes_)
        {
            if (n->type() != value_type_traits<T>::value)
            {
                return false;
            }
        }
        return true;
    }

    /// \brief	Returns true if this array contains only tables.
    bool is_table_array() const noexcept override
    {
        return is_homogeneous<toml::table>();
    }

    std::vector<std::shared_ptr<node>> &get() noexcept
    {
        return nodes_;
    }

    const std::vector<std::shared_ptr<node>> &get() const noexcept
    {
        return nodes_;
    }

    std::shared_ptr<node> at(size_t idx) const
    {
        return nodes_.at(idx);
    }

    std::shared_ptr<node> operator[](size_t index)
    {
        return nodes_[index];
    }

    std::shared_ptr<node> operator[](size_t index) const
    {
        return nodes_[index];
    }

    std::shared_ptr<node> front()
    {
        return nodes_.front();
    }

    std::shared_ptr<node> back()
    {
        return nodes_.back();
    }

    template <typename T>
    std::vector<T> collect() const
    {
        std::vector<T> result;
        for (const auto &n : nodes_)
        {
            if (const auto val = n->value<T>())
            {
                result.emplace_back(val.value());
            }
        }
        return result;
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>>
    std::vector<U> map_collect(F f) const
    {
        std::vector<U> result;
        for (const auto &n : nodes_)
        {
            if (const auto val = n->value<T>())
            {
                result.emplace_back(f(val.value()));
            }
        }
        return result;
    }

    void push_back(std::shared_ptr<node> &&n)
    {
        nodes_.emplace_back(n);
    }

    template <class T>
    void emplace_back(std::enable_if_t<toml::is_value<T>, T> &&val)
    {
        nodes_.emplace_back(make_value(std::forward<T>(val)));
    }

    void pop_back()
    {
        nodes_.pop_back();
    }

    void clear() noexcept
    {
        nodes_.clear();
    }

    iterator insert(iterator position, std::shared_ptr<node> &&value)
    {
        return nodes_.insert(position, value);
    }

    iterator erase(const_iterator pos)
    {
        return nodes_.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        return nodes_.erase(first, last);
    }

private:
    std::vector<std::shared_ptr<node>> nodes_;

    array() noexcept
        : node(base_type::Array) {}

    template <class InputIterator>
    array(InputIterator begin, InputIterator end) noexcept
        : node(base_type::Array),
          nodes_{begin, end} {}

    //array(array &&other) noexcept
    //    : {}

    array(const array &obj) = delete;
    array &operator=(const array &obj) = delete;

    /*

    /// \brief	Gets the node at a specific index.
    ///
    /// \detail \cpp
    /// auto arr = toml::array{ 99, "bottles of beer on the wall" };
    ///	std::cout << "node [0] exists: "sv << !!arr.get(0) << std::endl;
    ///	std::cout << "node [1] exists: "sv << !!arr.get(1) << std::endl;
    ///	std::cout << "node [2] exists: "sv << !!arr.get(2) << std::endl;
    /// if (auto val = arr.get(0))
    ///		std::cout << "node [0] was an "sv << val->type() << std::endl;
    ///
    /// \ecpp
    ///
    /// \out
    /// node [0] exists: true
    /// node [1] exists: true
    /// node [2] exists: false
    /// node [0] was an integer
    /// \eout
    ///
    /// \param 	index	The node's index.
    ///
    /// \returns	A pointer to the node at the specified index if one existed, or nullptr.
    [[nodiscard]] node *get(size_t index) noexcept;

    /// \brief	Gets the node at a specific index (const overload).
    ///
    /// \param 	index	The node's index.
    ///
    /// \returns	A pointer to the node at the specified index if one existed, or nullptr.
    [[nodiscard]] const node *get(size_t index) const noexcept;

    void preinsertion_resize(size_t idx, size_t count) noexcept;

    template <typename T>
    [[nodiscard]] static bool container_equality(const array &lhs, const T &rhs) noexcept
    {
        using elem_t = std::remove_const_t<typename T::value_type>;
        static_assert(
            impl::is_value_or_promotable<elem_t>,
            "Container element type must be (or be promotable to) one of the TOML value types");

        if (lhs.size() != rhs.size())
            return false;
        if (rhs.size() == 0_sz)
            return true;

        size_t i{};
        for (auto &list_elem : rhs)
        {
            const auto elem = lhs.get_as<impl::promoted<elem_t>>(i++);
            if (!elem || *elem != list_elem)
                return false;
        }

        return true;
    }

    [[nodiscard]] size_t total_leaf_count() const noexcept;

public:

    template <typename Char>
    friend std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &, const array &);
    */
};

std::shared_ptr<array> make_array()
{
    return std::make_shared<array>(array::make_shared_enabler{});
}

TOML_NAMESPACE_END
} // namespace toml