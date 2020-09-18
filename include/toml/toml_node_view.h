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

    node_view(std::shared_ptr<node> &&n) noexcept
        : node_{n} {}

    explicit operator bool() const noexcept
    {
        return node_ != nullptr;
    }

    base_type type() const noexcept
    {
        return node_ ? node_->type() : base_type::None;
    }

    const node &get() const noexcept
    {
        return *node_;
    }

    template <typename T>
    bool is() const noexcept
    {
        return node_ && node_->template is<T>();
    }

    bool is_value() const noexcept
    {
        return node_ && node_->is_value();
    }

    bool is_table() const noexcept
    {
        return node_ && node_->is<table>();
    };

    bool is_array() const noexcept
    {
        return node_ && node_->is<array>();
    }

    bool is_table_array() const noexcept
    {
        return node_ && node_->is_table_array();
    }

    bool contains(std::string_view key) const
    {
        auto position = key.find('.');
        if (position != std::string_view::npos && position + 1 < key.size())
        {
            return (*this)[key.substr(0, position)].contains(key.substr(position + 1));
        }
        else
        {
            return bool((*this)[key.substr(0, position)]);
        }
    }

    template <class T>
    auto as_value() const
    {
        return node_ ? node_->template as_value<T>() : nullptr;
    }

    template <class T>
    auto as() const
    {
        using return_type = decltype(node_->template as<T>());
        return node_ ? node_->as<T>() : return_type{};
    }

    template <class T>
    auto as()
    {
        using return_type = decltype(node_->template as<T>());
        return node_ ? node_->as<T>() : return_type{};
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

    template <typename T>
    auto value_or_default() const noexcept
    {
        using return_type = decltype(node_->value_or_default<T>());
        return node_ ? node_->value_or_default<T>() : return_type{};
    }

    node_view operator[](std::string_view key) const
    {
        node_view result{nullptr};
        auto position = key.find('.');

        if (auto tbl = this->as<table>())
        {
            result = node_view(tbl->at(key.substr(0, position)));
        }

        if (position != std::string_view::npos && position + 1 < key.size())
        {
            return result[key.substr(position + 1)];
        }
        else
        {
            return result;
        }
    }

    node_view operator[](size_t index) const
    {
        if (auto arr = this->as<array>())
        {
            return {(*arr)[index]};
        }
        else
        {
            return {nullptr};
        }
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<!std::is_void_v<U>>>
    std::optional<U> map(F &&f) const
    {
        return node_ ? node_->template map<T>(f) : std::nullopt;
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<std::is_void_v<U>>>
    void map(F &&f) const
    {
        if (node_)
        {
            node_->template map<T>(f);
        }
    }

    template <typename T, typename U = typename value_type_traits<T>::type>
    std::vector<U> collect() const
    {
        if (auto arr = this->as<array>())
        {
            return arr->template collect<T>();
        }
        else
        {
            return {};
        }
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F, const T &>,
              typename = std::enable_if_t<!std::is_void_v<U>>>
    std::vector<U> map_collect(F &&f) const
    {
        if (auto arr = this->as<array>())
        {
            return arr->template map_collect<T>(f);
        }
        else
        {
            return {};
        }
    }

    template <class Visitor, class... Args>
    void accept(Visitor &&visitor, Args &&... args) const
    {
        if (node_)
        {
            return node_->accept(std::forward<Visitor>(visitor), std::forward<Args>(args)...);
        }
    }

private:
    std::shared_ptr<node> node_;
};

node_view node::view() const noexcept
{
    return node_view(std::const_pointer_cast<node>(shared_from_this()));
}

TOML_NAMESPACE_END
} // namespace toml