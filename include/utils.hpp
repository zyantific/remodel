#ifndef _REMODEL_UTILS_HPP_
#define _REMODEL_UTILS_HPP_

#include <type_traits>
#include <cstdint>

namespace remodel
{

using Flags = uint32_t;

namespace utils
{

// ============================================================================================== //
// TMP tools                                                                                      //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [BlackBoxConsts]                                                                               //
// ---------------------------------------------------------------------------------------------- //

/**  
 * @brief   Black-box constants opaque to the compiler until template instantiation.
 *          
 * Can be used to make @c static_assert statements evaluate only when the containing template
 * is about to be instantiated. Some people use @c sizeof(T)\ !=\ sizeof(T) instead, which 
 * however, other than this version, fails for types that cannot be sizeofed (e.g. functions).
 * Lazy evaluation is accomplished by making the constants dependant on a template argument.
 */
template<typename>
struct BlackBoxConsts
{
    enum
    {
        false_ = false,
        true_ = true,
    };
};

// ---------------------------------------------------------------------------------------------- //
// [CloneConst]                                                                                   //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Clones the const qualifier from one type to another.
 * @tparam  SrcT    Source type.
 * @tparam  DstT    Destination type.
 */
template<typename SrcT, typename DstT>
struct CloneConst
{
    using type = std::conditional_t<
        std::is_const<SrcT>::value, 
        std::add_const_t<DstT>,
        std::remove_const_t<DstT>
    >;
};

// ---------------------------------------------------------------------------------------------- //
// [InheritIfFlags]                                                                               //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{
    template<typename T, bool doInheritT>
    struct InheritIfHelper {};

    template<typename T>
    struct InheritIfHelper<T, true> : T {};
} // namespace internal

/**
 * @brief   Inherits @c T if @c flagsT\ &\ flagConditionT evaluates to @c true.
 * @tparam  flagsT          The flags.
 * @tparam  flagConditionT  The flags that need to be @c true to inherit.
 * @tparam  T               The class to inherit if the required flags are set.
 */
template<Flags flagsT, Flags flagConditionT, typename T>
struct InheritIfFlags 
    : internal::InheritIfHelper<T, (flagsT & flagConditionT) == flagConditionT> {};

// ============================================================================================== //
// Mix-ins                                                                                        //
// ============================================================================================== //

// ---------------------------------------------------------------------------------------------- //
// [NonCopyable]                                                                                  //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Makes deriving classes non-copyable.
 */
class NonCopyable
{
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator = (const NonCopyable&) = delete;
};

// ============================================================================================== //
// Misc                                                                                           //
// ============================================================================================== //

static const struct EmptyT {} Empty;

inline void fatalExit(const char* /*why*/)
{
    std::terminate();
}

template<typename T>
class Optional final
{
    bool m_hasValue;
    uint8_t m_data[sizeof(T)];

    T* ptr()             { return reinterpret_cast<T*>(m_data); }
    const T* ptr() const { return reinterpret_cast<const T*>(m_data); }

    void destroyValue()
    {
        if (m_hasValue)
        {
            ptr()->~T();
            m_hasValue = false;
        }
    }
public:
    using ValueType = T;

    Optional(EmptyT) : m_hasValue(false) {}

    template<typename FirstArgT, typename... OtherArgsT>
    Optional(FirstArgT firstArg, OtherArgsT... otherArgs)
    {
        new (m_data) T{firstArg, otherArgs...};
    }

    template<typename TT=T, std::enable_if_t<std::is_copy_constructible<TT>::value>* = 0>
    Optional(T& value)
        : m_hasValue(true)
    {
        new (m_data) T{value};
    }

    template<typename TT=T, std::enable_if_t<std::is_move_constructible<TT>::value>* = 0>
    Optional(T&& value)
        : m_hasValue(true)
    {
        new (m_data) T{value};
    }

    template<typename TT=T, std::enable_if_t<std::is_copy_constructible<TT>::value>* = 0>
    Optional(const Optional<T>& copyFrom)
        : m_hasValue(copyFrom.m_hasValue)
    {
        if (copyFrom.m_hasValue) 
        {
            new (m_data) T{*copyFrom.ptr()};
        }
    }

    template<typename TT=T, std::enable_if_t<std::is_copy_constructible<TT>::value 
        && std::is_copy_assignable<TT>::value >* = 0>
    Optional<T>& operator = (const Optional<T>& copyFrom)
    {
        if (copyFrom.m_hasValue)
        {
            if (m_hasValue)
            {
                *ptr() = *copyFrom.ptr();
            }
            else
            {
                new (m_data) T{*copyFrom.ptr()};
            }
        }
        else
        {
            destroyValue();
        }

        m_hasValue = copyFrom.m_hasValue;
        return *this;
    }

    template<typename TT=T, std::enable_if_t<std::is_move_constructible<TT>::value>* = 0>
    Optional(Optional<T>&& moveFrom)
        : m_hasValue(moveFrom.m_hasValue)
    {
        if (moveFrom.m_hasValue) 
        {
            new (m_data) T{*std::move(moveFrom.ptr())};
            moveFrom.destroyValue();
        }
    }

    template<typename TT=T, std::enable_if_t<std::is_move_constructible<TT>::value 
        && std::is_move_assignable<TT>::value >* = 0>
    Optional<T>& operator = (Optional<T>&& moveFrom)
    {
        if (moveFrom.m_hasValue)
        {
            if (m_hasValue)
            {
                *ptr() = std::move(*moveFrom.ptr());
            }
            else
            {
                new (m_data) T{*std::move(moveFrom.ptr())};
            }

            moveFrom.destroyValue();
        }
        else
        {
            destroyValue();
        }

        m_hasValue = moveFrom.m_hasValue;
        return *this;
    }

    ~Optional()
    {
        destroyValue();
    }

    bool hasValue() const { return m_hasValue; }

    T& value()
    {
        if (!m_hasValue) fatalExit("tried to retrieve value of Optional without value");
        return *ptr();
    }

    template<std::enable_if_t<std::is_move_assignable<T>::value>* = 0>
    T&& release()
    {
        if (!m_hasValue) fatalExit("tried to release value of Optional without value");

        m_hasValue = false;
        return std::move(*ptr());
    }
};

// ============================================================================================== //

} // namespace utils
} // namespace remodel

#endif // _REMODEL_UTILS_HPP_