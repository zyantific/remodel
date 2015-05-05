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

// ---------------------------------------------------------------------------------------------- //
// [IsMovable]                                                                                    //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
struct IsMovable
    : std::integral_constant<
        bool, 
        std::is_move_assignable<T>::value && std::is_move_constructible<T>::value
    > 
{};

// ---------------------------------------------------------------------------------------------- //
// [IsCopyable]                                                                                   //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
struct IsCopyable
    : std::integral_constant<
        bool,
        std::is_copy_assignable<T>::value && std::is_copy_constructible<T>::value
    >
{};

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
// Optional                                                                                       //
// ============================================================================================== //

static const struct EmptyT { EmptyT() {} } Empty;
static const struct InPlaceT { InPlaceT() {} } InPlace;

inline void fatalExit(const char* /*why*/)
{
    std::terminate();
}

namespace internal
{

// ---------------------------------------------------------------------------------------------- //
// [OptionalImplBase]                                                                             //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
class OptionalImplBase
{
    bool m_hasValue;
    std::aligned_storage_t<sizeof(T), std::alignment_of<T>::value> m_data;

    T* ptr()             { return reinterpret_cast<T*>(&m_data); }
    const T* ptr() const { return reinterpret_cast<const T*>(&m_data); }

    void destroyValue()
    {
        if (m_hasValue)
        {
            ptr()->~T();
            m_hasValue = false;
        }
    }
protected: // Copy and move semantics (used by OptionalImpl)
    template<typename TT=T, std::enable_if_t<IsMovable<TT>::value, int> = 0>
    void moveConstruct(OptionalImplBase<T>&& other)
    {
        m_hasValue = other.m_hasValue;
        if (other.m_hasValue) 
        {
            new (ptr()) T{std::move(*other.ptr())};
            other.destroyValue();
        }
    }

    template<typename TT=T, std::enable_if_t<IsMovable<TT>::value, int> = 0>
    void moveAssign(OptionalImplBase<T>&& other)
    {
        if (other.m_hasValue)
        {
            if (m_hasValue)
            {
                *ptr() = std::move(*other.ptr());
            }
            else
            {
                new (ptr()) T{std::move(*other.ptr())};
            }

            other.destroyValue();
        }
        else
        {
            destroyValue();
        }

        m_hasValue = other.m_hasValue;
    }

    template<typename TT=T, std::enable_if_t<IsCopyable<TT>::value, int> = 0>
    void copyConstruct(const OptionalImplBase<T>& other)
    {
        m_hasValue = other.m_hasValue;
        if (other.m_hasValue) 
        {
            new (ptr()) T{*other.ptr()};
        }
    }

    template<typename TT=T, std::enable_if_t<IsCopyable<TT>::value, int> = 0>
    void copyAssign(const OptionalImplBase<T>& other)
    {
        if (other.m_hasValue)
        {
            if (m_hasValue)
            {
                *ptr() = *other.ptr();
            }
            else
            {
                new (ptr()) T{*other.ptr()};
            }
        }
        else
        {
            destroyValue();
        }

        m_hasValue = other.m_hasValue;
    }

    template<typename TT=T, std::enable_if_t<IsMovable<TT>::value, int> = 0>
    void moveConstruct(T&& obj)
    {
        m_hasValue = true;
        new (ptr()) T{std::forward<T>(obj)};
    }

    template<typename TT=T, std::enable_if_t<IsMovable<TT>::value, int> = 0>
    void moveAssign(T&& obj)
    {
        destroyValue();
        new (ptr()) T{std::forward<T>(obj)};
        m_hasValue = true;
    }

    template<typename TT=T, std::enable_if_t<IsCopyable<TT>::value, int> = 0>
    void copyConstruct(const T& obj)
    {
        m_hasValue = true;
        new (ptr()) T{obj};
    }

    template<typename TT=T, std::enable_if_t<IsCopyable<TT>::value, int> = 0>
    void copyAssign(const T& obj)
    {
        destroyValue();
        new (ptr()) T{obj};
        m_hasValue = true;
    }
public: // Member functions
    using ValueType = T;

    OptionalImplBase() : OptionalImplBase(Empty) {}
    OptionalImplBase(EmptyT) : m_hasValue(false) {}

    template<typename... ArgsT>
    OptionalImplBase(InPlaceT, ArgsT... args)
    {
        new (ptr()) T{args...};
    }

    ~OptionalImplBase()
    {
        destroyValue();
    }
public: // Observers
    bool hasValue() const { return m_hasValue; }
    operator bool () const { return hasValue(); }

    T& value()
    {
        if (!m_hasValue) fatalExit("tried to retrieve value of Optional without value");
        return *ptr();
    }

    template<typename TT=T, std::enable_if_t<IsMovable<TT>::value, int> = 0>
    T release()
    {
        auto tmp = std::move(value());
        destroyValue();
        return tmp;
    }
};

// ---------------------------------------------------------------------------------------------- //
// [OptionalImpl] for non-copyable, non-movable types                                             //
// ---------------------------------------------------------------------------------------------- //

#define REMODEL_OPTIONAL_FWD_INPLACE_CTORS                                                         \
    OptionalImpl() : OptionalImplBase<T>(Empty) {}                                                 \
    OptionalImpl(EmptyT) : OptionalImplBase<T>(Empty) {}                                           \
    template<typename... ArgsT>                                                                    \
    OptionalImpl(InPlaceT, ArgsT... args) : OptionalImplBase<T>(InPlace, args...) {}

#define REMODEL_OPTIONAL_IMPL_MOVE_CTORS                                                           \
    OptionalImpl(OptionalImpl&& other)                                                             \
        { this->moveConstruct(std::forward<OptionalImpl>(other)); }                                \
    OptionalImpl& operator = (OptionalImpl&& other)                                                \
        { this->moveAssign(std::forward<OptionalImpl>(other)); return *this; }                     \
    OptionalImpl(T&& other)                                                                        \
        { this->moveConstruct(std::forward<T>(other)); }                                           \
    OptionalImpl& operator = (T&& other)                                                           \
        { this->moveAssign(std::forward<T>(other)); return *this; }

#define REMODEL_OPTIONAL_IMPL_COPY_CTORS                                                           \
    OptionalImpl(const OptionalImpl& other)                                                        \
        { this->copyConstruct(other); }                                                            \
    OptionalImpl& operator = (const OptionalImpl& other)                                           \
        { this->copyAssign(other); return *this; }                                                 \
    OptionalImpl(const T& other)                                                                   \
        { this->copyConstruct(other); }                                                            \
    OptionalImpl& operator = (const T& other)                                                      \
        { this->copyAssign(other); return *this; }

template<typename T, typename=void>
struct OptionalImpl 
    : OptionalImplBase<T>
{
    REMODEL_OPTIONAL_FWD_INPLACE_CTORS
};

// ---------------------------------------------------------------------------------------------- //
// [OptionalImpl] for non-copyable, movable types                                                 //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
struct OptionalImpl<T, std::enable_if_t<!IsCopyable<T>::value && IsMovable<T>::value>>
     : OptionalImplBase<T>
{
    REMODEL_OPTIONAL_FWD_INPLACE_CTORS
    REMODEL_OPTIONAL_IMPL_MOVE_CTORS
};

// ---------------------------------------------------------------------------------------------- //
// [OptionalImpl] for copyable, non-movable types                                                 //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
struct OptionalImpl<T, std::enable_if_t<IsCopyable<T>::value && !IsMovable<T>::value>>
     : OptionalImplBase<T>
{
    REMODEL_OPTIONAL_FWD_INPLACE_CTORS
    REMODEL_OPTIONAL_IMPL_COPY_CTORS
};

// ---------------------------------------------------------------------------------------------- //
// [OptionalImpl] for copyable, movable types                                                     //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
struct OptionalImpl<T, std::enable_if_t<IsCopyable<T>::value && IsMovable<T>::value>>
     : OptionalImplBase<T>
{
    REMODEL_OPTIONAL_FWD_INPLACE_CTORS
    REMODEL_OPTIONAL_IMPL_MOVE_CTORS
    REMODEL_OPTIONAL_IMPL_COPY_CTORS
};

#undef REMODEL_OPTIONAL_FWD_INPLACE_CTORS
#undef REMODEL_OPTIONAL_IMPL_COPY_CTORS
#undef REMODEL_OPTIONAL_IMPL_MOVE_CTORS

} // namespace internal

// ---------------------------------------------------------------------------------------------- //
// [Optional]                                                                                     //
// ---------------------------------------------------------------------------------------------- //

template<typename T>
using Optional = internal::OptionalImpl<T>;

// ============================================================================================== //

} // namespace utils
} // namespace remodel

#endif // _REMODEL_UTILS_HPP_