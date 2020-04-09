/**
 * A concrete TOML value representing the "leaves" of the "tree".
 */
template <class T>
class value final : public base
{
    static_assert(std::is_same_v<T, base_type_traits<T>::type>,
                  "Template type parameter must be one of the TOML value types");

public:
    using value_type = T;

    virtual base_type type() const noexcept override
    {
        return base_type_traits<T>::value;
    }

    /// \brief	A type alias for 'value arguments'.
    /// \details This differs according to the value's type argument:
    /// 		 - ints, floats, booleans: `value_type`
    /// 		 - strings: `string_view`
    /// 		 - everything else: `const value_type&`
    using value_arg = std::conditional_t<std::is_same_v<T, std::string>,
                                         string_view,
                                         std::conditional_t<is_one_of<T, double, int64_t, bool>,
                                                            T,
                                                            const T &>>;

    std::shared_ptr<base> clone() const override;

    value(const make_shared_enabler &, const T &val) : value(val)
    {
        // nothing; note that users cannot actually invoke this function
        // because they lack access to the make_shared_enabler.
    }

    bool is_value() const override
    {
        return true;
    }

    /**
     * Gets the data associated with this value.
     */
    T &get()
    {
        return data_;
    }

    /**
     * Gets the data associated with this value. Const version.
     */
    const T &get() const
    {
        return data_;
    }

private:
    struct make_shared_enabler
    {
        // nothing; this is a private key accessible only to friends
    };

    template <class U>
    friend std::shared_ptr<typename value_traits<U>::type>
    cpptoml::make_value(U &&val);

    T data_;

    /**
     * Constructs a value from the given data.
     */

    value(const T &val) : base(base_type_traits<T>::type), data_(val)
    {
    }

    value(const value &val) = delete;
    value &operator=(const value &val) = delete;
};

template <class T>
std::shared_ptr<typename value_traits<T>::type> make_value(T &&val)
{
    using value_type = typename value_traits<T>::type;
    using enabler = typename value_type::make_shared_enabler;
    return std::make_shared<value_type>(
        enabler{}, value_traits<T>::construct(std::forward<T>(val)));
}

template <class T>
inline std::shared_ptr<value<T>> base::as()
{
#if defined(CPPTOML_NO_RTTI)
    if (type() == base_type_traits<T>::type)
        return std::static_pointer_cast<value<T>>(shared_from_this());
    else
        return nullptr;
#else
    return std::dynamic_pointer_cast<value<T>>(shared_from_this());
#endif
}