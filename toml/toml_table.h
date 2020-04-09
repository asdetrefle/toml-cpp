#pragma once

#include "toml_array.h"

namespace toml
{
TOML_NAMESPACE_BEGIN
/**
 * Represents a TOML keytable.
 */
class table : public node
{
public:
    friend class table_array;
    friend std::shared_ptr<table> make_table();

    std::shared_ptr<base> clone() const override;

    /**
     * tables can be iterated over.
     */
    using iterator = string_to_base_map::iterator;

    /**
     * tables can be iterated over. Const version.
     */
    using const_iterator = string_to_base_map::const_iterator;

    iterator begin()
    {
        return map_.begin();
    }

    const_iterator begin() const
    {
        return map_.begin();
    }

    iterator end()
    {
        return map_.end();
    }

    const_iterator end() const
    {
        return map_.end();
    }

    bool is_table() const override
    {
        return true;
    }

    bool empty() const
    {
        return map_.empty();
    }

    /**
     * Determines if this key table contains the given key.
     */
    bool contains(const std::string &key) const
    {
        return map_.find(key) != map_.end();
    }

    /**
     * Determines if this key table contains the given key. Will
     * resolve "qualified keys". Qualified keys are the full access
     * path separated with dots like "grandparent.parent.child".
     */
    bool contains_qualified(const std::string &key) const
    {
        return resolve_qualified(key);
    }

    /**
     * Obtains the base for a given key.
     * @throw std::out_of_range if the key does not exist
     */
    std::shared_ptr<base> get(const std::string &key) const
    {
        return map_.at(key);
    }

    /**
     * Obtains the base for a given key. Will resolve "qualified
     * keys". Qualified keys are the full access path separated with
     * dots like "grandparent.parent.child".
     *
     * @throw std::out_of_range if the key does not exist
     */
    std::shared_ptr<base> get_qualified(const std::string &key) const
    {
        std::shared_ptr<base> p;
        resolve_qualified(key, &p);
        return p;
    }

    /**
     * Obtains a table for a given key, if possible.
     */
    std::shared_ptr<table> get_table(const std::string &key) const
    {
        if (contains(key) && get(key)->is_table())
            return std::static_pointer_cast<table>(get(key));
        return nullptr;
    }

    /**
     * Obtains a table for a given key, if possible. Will resolve
     * "qualified keys".
     */
    std::shared_ptr<table> get_table_qualified(const std::string &key) const
    {
        if (contains_qualified(key) && get_qualified(key)->is_table())
            return std::static_pointer_cast<table>(get_qualified(key));
        return nullptr;
    }

    /**
     * Obtains an array for a given key.
     */
    std::shared_ptr<array> get_array(const std::string &key) const
    {
        if (!contains(key))
            return nullptr;
        return get(key)->as_array();
    }

    /**
     * Obtains an array for a given key. Will resolve "qualified keys".
     */
    std::shared_ptr<array> get_array_qualified(const std::string &key) const
    {
        if (!contains_qualified(key))
            return nullptr;
        return get_qualified(key)->as_array();
    }

    /**
     * Obtains a table_array for a given key, if possible.
     */
    std::shared_ptr<table_array> get_table_array(const std::string &key) const
    {
        if (!contains(key))
            return nullptr;
        return get(key)->as_table_array();
    }

    /**
     * Obtains a table_array for a given key, if possible. Will resolve
     * "qualified keys".
     */
    std::shared_ptr<table_array>
    get_table_array_qualified(const std::string &key) const
    {
        if (!contains_qualified(key))
            return nullptr;
        return get_qualified(key)->as_table_array();
    }

    /**
     * Helper function that attempts to get a value corresponding
     * to the template parameter from a given key.
     */
    template <class T>
    std::optional<T> get_as(const std::string &key) const
    {
        try
        {
            return get_impl<T>(get(key));
        }
        catch (const std::out_of_range &)
        {
            return {};
        }
    }

    /**
     * Helper function that attempts to get a value corresponding
     * to the template parameter from a given key. Will resolve "qualified
     * keys".
     */
    template <class T>
    std::optional<T> get_qualified_as(const std::string &key) const
    {
        try
        {
            return get_impl<T>(get_qualified(key));
        }
        catch (const std::out_of_range &)
        {
            return {};
        }
    }

    /**
     * Helper function that attempts to get an array of values of a given
     * type corresponding to the template parameter for a given key.
     *
     * If the key doesn't exist, doesn't exist as an array type, or one or
     * more keys inside the array type are not of type T, an empty optional
     * is returned. Otherwise, an optional containing a vector of the values
     * is returned.
     */
    template <class T>
    inline typename array_of_trait<T>::return_type
    get_array_of(const std::string &key) const
    {
        if (auto v = get_array(key))
        {
            std::vector<T> result;
            result.reserve(v->get().size());

            for (const auto &b : v->get())
            {
                if (auto val = b->as<T>())
                    result.push_back(val->get());
                else
                    return {};
            }
            return {std::move(result)};
        }

        return {};
    }

    /**
     * Helper function that attempts to get an array of values of a given
     * type corresponding to the template parameter for a given key. Will
     * resolve "qualified keys".
     *
     * If the key doesn't exist, doesn't exist as an array type, or one or
     * more keys inside the array type are not of type T, an empty optional
     * is returned. Otherwise, an optional containing a vector of the values
     * is returned.
     */
    template <class T>
    inline typename array_of_trait<T>::return_type
    get_qualified_array_of(const std::string &key) const
    {
        if (auto v = get_array_qualified(key))
        {
            std::vector<T> result;
            result.reserve(v->get().size());

            for (const auto &b : v->get())
            {
                if (auto val = b->as<T>())
                    result.push_back(val->get());
                else
                    return {};
            }
            return {std::move(result)};
        }

        return {};
    }

    /**
     * Adds an element to the keytable.
     */
    void insert(const std::string &key, const std::shared_ptr<base> &value)
    {
        map_[key] = value;
    }

    /**
     * Convenience shorthand for adding a simple element to the
     * keytable.
     */
    template <class T>
    void insert(const std::string &key, T &&val,
                typename value_traits<T>::type * = 0)
    {
        insert(key, make_value(std::forward<T>(val)));
    }

    /**
     * Removes an element from the table.
     */
    void erase(const std::string &key)
    {
        map_.erase(key);
    }

private:
#if defined(CPPTOML_NO_RTTI)
    table() : base(base_type::TABLE)
    {
        // nothing
    }
#else
    table()
    {
        // nothing
    }
#endif

    table(const table &obj) = delete;
    table &operator=(const table &rhs) = delete;

    std::vector<std::string> split(const std::string &value,
                                   char separator) const
    {
        std::vector<std::string> result;
        std::string::size_type p = 0;
        std::string::size_type q;
        while ((q = value.find(separator, p)) != std::string::npos)
        {
            result.emplace_back(value, p, q - p);
            p = q + 1;
        }
        result.emplace_back(value, p);
        return result;
    }

    // If output parameter p is specified, fill it with the pointer to the
    // specified entry and throw std::out_of_range if it couldn't be found.
    //
    // Otherwise, just return true if the entry could be found or false
    // otherwise and do not throw.
    bool resolve_qualified(const std::string &key,
                           std::shared_ptr<base> *p = nullptr) const
    {
        auto parts = split(key, '.');
        auto last_key = parts.back();
        parts.pop_back();

        auto cur_table = this;
        for (const auto &part : parts)
        {
            cur_table = cur_table->get_table(part).get();
            if (!cur_table)
            {
                if (!p)
                    return false;

                throw std::out_of_range{key + " is not a valid key"};
            }
        }

        if (!p)
            return cur_table->map_.count(last_key) != 0;

        *p = cur_table->map_.at(last_key);
        return true;
    }

    string_to_base_map map_;
};
TOML_NAMESPACE_END
} // namespace toml