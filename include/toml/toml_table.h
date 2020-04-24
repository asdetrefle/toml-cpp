#pragma once

#include "toml_base.h"
#include "toml_node.h"

namespace toml
{
TOML_NAMESPACE_BEGIN
/**
 * Represents a TOML keytable.
 */
class table final : public node
{
    struct make_shared_enabler
    {
    };
    friend std::shared_ptr<table> make_table(bool is_inline);

public:
    using map = std::map<std::string, std::shared_ptr<node>, std::less<>>;

    using key_type = std::string;
    using value_type = std::shared_ptr<node>;
    using size_type = size_t;

    using iterator = map::iterator;
    using const_iterator = map::const_iterator;

    table(const make_shared_enabler &, bool is_inline) noexcept
        : node(base_type::Table),
          is_inline_(is_inline) {}

    std::shared_ptr<node> clone() const override
    {
        auto result = make_table();
        for (const auto &pr : map_)
            result->emplace(pr.first, pr.second->clone());
        return result;
    }

    iterator begin() noexcept
    {
        return map_.begin();
    }

    const_iterator begin() const noexcept
    {
        return map_.begin();
    }

    const_iterator cbegin() const noexcept
    {
        return map_.cbegin();
    }

    iterator end() noexcept
    {
        return map_.end();
    }

    const_iterator end() const noexcept
    {
        return map_.end();
    }

    const_iterator cend() const noexcept
    {
        return map_.cend();
    }

    bool is_inline() const noexcept
    {
        return is_inline_;
    }

    bool empty() const noexcept
    {
        return map_.empty();
    }

    size_t size() const noexcept
    {
        return map_.size();
    }

    bool contains(std::string_view key) const
    {
        return map_.find(key) != map_.end();
    }

    map &get() noexcept
    {
        return map_;
    }

    const map &get() const noexcept
    {
        return map_;
    }

    std::shared_ptr<node> at(std::string_view key)
    {
        if (auto it = map_.find(key); it != map_.end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }

    std::shared_ptr<const node> at(std::string_view key) const
    {
        if (auto it = map_.find(key); it != map_.end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }

    iterator find(std::string_view key)
    {
        return map_.find(key);
    }

    const_iterator find(std::string_view key) const
    {
        return map_.find(key);
    }

    // this will overwrite existing node
    template <typename K, typename V, typename = std::enable_if_t<std::is_convertible_v<K &&, std::string_view>>>
    std::pair<iterator, bool> insert_or_assign(K &&key, V &&val) noexcept
    {
        return map_.insert_or_assign(std::forward<K>(key), std::forward<V>(val));
    }

    // this will not overwrite existing node
    template <typename K, typename V, typename = std::enable_if_t<std::is_convertible_v<K &&, std::string_view>>>
    std::pair<iterator, bool> emplace(K &&key, V &&val)
    {
        auto ipos = map_.lower_bound(key);
        if (ipos == map_.end() || ipos->first != key)
        {
            if constexpr (is_value_promotable<V>)
            {
                ipos = map_.emplace_hint(ipos, std::forward<K>(key), make_value(std::forward<V>(val)));
            }
            else
            {
                ipos = map_.emplace_hint(ipos, std::forward<K>(key), std::forward<V>(val));
            }
            return {ipos, true};
        }
        return {ipos, false};
    }

    iterator erase(iterator pos)
    {
        return map_.erase(pos);
    }

    iterator erase(const_iterator pos)
    {
        return map_.erase(pos);
    }

    bool erase(std::string_view key)
    {
        if (auto it = map_.find(key); it != map_.end())
        {
            map_.erase(it);
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    map map_;
    bool is_inline_{false};

    table(bool is_inline)
        : node(base_type::Table),
          is_inline_(is_inline) {}

    table(const table &obj) = delete;
    table &operator=(const table &rhs) = delete;
};

std::shared_ptr<table> make_table(bool is_inline)
{
    return std::make_shared<table>(table::make_shared_enabler{}, is_inline);
}
TOML_NAMESPACE_END
} // namespace toml