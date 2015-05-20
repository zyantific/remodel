/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 athre0z
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _REMODEL_UTILS_HPP_
#define _REMODEL_UTILS_HPP_

#include <type_traits>
#include <cstdint>
#include <utility>

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
    static const bool kFalse = false;
    static const bool kTrue = true;
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
using CloneConst = std::conditional_t<
    std::is_const<SrcT>::value, 
    std::add_const_t<DstT>,
    std::remove_const_t<DstT>
>;

// ---------------------------------------------------------------------------------------------- //
// [CloneVolatile]                                                                                //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Clones the volatile qualifier from one type to another.
 * @tparam  SrcT    Source type.
 * @tparam  DstT    Destination type.
 */
template<typename SrcT, typename DstT>
using CloneVolatile = std::conditional_t<
    std::is_volatile<SrcT>::value, 
    std::add_volatile_t<DstT>,
    std::remove_volatile_t<DstT>
>;

// ---------------------------------------------------------------------------------------------- //
// [CloneCv]                                                                                      //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Clones the const and volatile  qualifiers from one type to another.
 * @tparam  SrcT    Source type.
 * @tparam  DstT    Destination type.
 */
template<typename SrcT, typename DstT>
using CloneCv = typename CloneVolatile<SrcT, CloneConst<SrcT, DstT>>::Type;

// ---------------------------------------------------------------------------------------------- //
// [InheritIfFlags]                                                                               //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

template<typename T, bool doInheritT>
struct InheritIfImpl {};

template<typename T>
struct InheritIfImpl<T, true> : T {};

} // namespace internal

/**
 * @brief   Inherits @c T if @c flagsT\ &\ flagConditionT evaluates to @c true.
 * @tparam  flagsT          The flags.
 * @tparam  flagConditionT  The flags that need to be @c true to inherit.
 * @tparam  T               The class to inherit if the required flags are set.
 */
template<Flags flagsT, Flags flagConditionT, typename T>
using InheritIfFlags 
    = internal::InheritIfImpl<T, (flagsT & flagConditionT) == flagConditionT>;

//template<Flags flagsT, Flags flagConditionT, typename T>
//struct InheritIfFlags 
//    : internal::InheritIfImpl<T, (flagsT & flagConditionT) == flagConditionT> {};

// ---------------------------------------------------------------------------------------------- //
// [IsMovable]                                                                                    //
// ---------------------------------------------------------------------------------------------- //

/**
 * @brief   Determines if a type is move assign and constructable.
 * @tparam  T   The type to check.
 */
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

// ---------------------------------------------------------------------------------------------- //
// [TypeStack]                                                                                    //
// ---------------------------------------------------------------------------------------------- //

template<typename... ElementsT>
struct TypeStack final
{
    /**   
     * @brief   Bottom type indicating that there are no more elements on the stack.
     */
    class Bottom {};
private:
    /**
     * @brief   Push implementation.
     * @tparam  ItemT   The new item to push.
     */
    template<typename ItemT>
    struct PushImpl
    {
        using NewStack = TypeStack<ItemT, ElementsT...>;
    };

    /**
     * @brief   Pop implementation capturing on empty stacks.
     * @tparam  TypeStackT  Type stack type.
     */
    template<typename TypeStackT>
    struct PopImpl
    {
        using NewStack = TypeStackT;
        using Item = Bottom;
    };

    /**
     * @brief   Pop implementation capturing on stacks with elements.
     * @tparam  TypeStackT  Type stack type.
     */
    template<template<typename...> class TypeStackT, typename... ContentT, typename TopItemT>
    struct PopImpl<TypeStackT<TopItemT, ContentT...>>
    {
        using NewStack = TypeStackT<ContentT...>;
        using Item = TopItemT;
    };
public:
    template<typename ItemT>
    using Push = typename PushImpl<ItemT>::NewStack;

    using Pop = typename PopImpl<TypeStack<ElementsT...>>::NewStack;
    using Top = typename PopImpl<TypeStack<ElementsT...>>::Item;

    using SizeType = std::size_t;
    static const SizeType kSize = sizeof...(ElementsT);
    
    static const bool kEmpty = kSize == 0;
};

// ---------------------------------------------------------------------------------------------- //
// [AnalyzeQualifiers]                                                                            //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

// Final layer implementation
template<typename T, typename LayerStackT>
struct AnalyzeQualifiersFinalImpl
{
    using QualifierStack = LayerStackT;

    using BaseType = T;

    using DepthType = typename LayerStackT::SizeType;
    static const DepthType kDepth = LayerStackT::kSize;
};

// Plain type (+ CV)
template<typename T, typename LayerStackT>
struct AnalyzeQualifiersImpl
    : AnalyzeQualifiersFinalImpl<T, LayerStackT> {};

template<typename T, typename LayerStackT>
struct AnalyzeQualifiersImpl<const T, LayerStackT> 
    : AnalyzeQualifiersFinalImpl<const T, LayerStackT> {};

template<typename T, typename LayerStackT>
struct AnalyzeQualifiersImpl<volatile T, LayerStackT> 
    : AnalyzeQualifiersFinalImpl<volatile T, LayerStackT> {};

template<typename T, typename LayerStackT>
struct AnalyzeQualifiersImpl<const volatile T, LayerStackT> 
    : AnalyzeQualifiersFinalImpl<const volatile T, LayerStackT> {};

#define REMODEL_ANALYZEQUALIFIERS_SPEC(spec)                                                       \
    template<typename T, typename LayerStackT>                                                     \
    struct AnalyzeQualifiersImpl<T spec, LayerStackT>                                              \
        : AnalyzeQualifiersImpl<T, typename LayerStackT::template Push<int spec>> {};

// Pointer layers (+ CV)
REMODEL_ANALYZEQUALIFIERS_SPEC(*)
REMODEL_ANALYZEQUALIFIERS_SPEC(* const)
REMODEL_ANALYZEQUALIFIERS_SPEC(* volatile)
REMODEL_ANALYZEQUALIFIERS_SPEC(* const volatile)

// lvalue-reference layers
REMODEL_ANALYZEQUALIFIERS_SPEC(&)

// rvalue-reference layers
REMODEL_ANALYZEQUALIFIERS_SPEC(&&)

// Unknown-size arrays
REMODEL_ANALYZEQUALIFIERS_SPEC([])

#undef REMODEL_ANALYZEQUALIFIERS_SPEC

// Known-size arrays
template<typename T, typename LayerStackT, std::size_t N>
struct AnalyzeQualifiersImpl<T[N], LayerStackT>
    : AnalyzeQualifiersImpl<T, typename LayerStackT::template Push<int[N]>> {};

} // namespace internal

/**
 * @brief   Analyzes a type's qualifiers
 * @tparam  T   The subject of the analysis.
 *              
 * All qualifier-levels are processed and pushed onto a stack (@c QualifierStack) by applying them
 * to a holder-type (@c int). Pointer layers may also come with additional CV-qualifications. 
 * The base-type can be retrieved from @c BaseType, the amount of qualification layers is stored 
 * into the @c kDepth constant.
 * 
 * Example:
 * @code
 *      using Result = utils::AnalyzeQualifiers<float const (*volatile &) [42]>;
 *      std::cout << "BaseType: " << typeid(Result::BaseType).name() << std::endl;
 *      std::cout << "QualifierStack: " << typeid(Result::QualifierStack).name() << std::endl;
 *      std::cout << "Depth: " << static_cast<Result::DepthType>(Result::kDepth) << std::endl;
 *      
 *      // Output:
 *      // BaseType: float
 *      // QualifierStack: struct remodel::utils::TypeStack<int &,int * volatile,int const[42]>
 *      // Depth: 3
 * @endcode
 */
template<typename T>
using AnalyzeQualifiers = internal::AnalyzeQualifiersImpl<T, TypeStack<>>;

// ---------------------------------------------------------------------------------------------- //
// [CloneQualifiers]                                                                              //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{

// Plain types (+ CV)
template<typename DstT, typename SrcT>
struct CloneQualifierImpl
{
    using Type = DstT;
};

template<typename DstT, typename SrcT>
struct CloneQualifierImpl<DstT, const SrcT>
{
    using Type = const DstT;
};

template<typename DstT, typename SrcT>
struct CloneQualifierImpl<DstT, volatile SrcT>
{
    using Type = volatile DstT;
};

template<typename DstT, typename SrcT>
struct CloneQualifierImpl<DstT, const volatile SrcT>
{
    using Type = const volatile DstT;
};

#define REMODEL_CLONEQUALIFIER_SPEC(spec)                                                          \
    template<typename DstT, typename SrcT>                                                         \
    struct CloneQualifierImpl<DstT, SrcT spec>                                                     \
    {                                                                                              \
        using Type = DstT spec;                                                                    \
    };

// Pointer layers (+ CV)
REMODEL_CLONEQUALIFIER_SPEC(*);
REMODEL_CLONEQUALIFIER_SPEC(* const);
REMODEL_CLONEQUALIFIER_SPEC(* volatile);
REMODEL_CLONEQUALIFIER_SPEC(* const volatile);

// lvalue-qualifiers
REMODEL_CLONEQUALIFIER_SPEC(&)

// rvalue-qualifiers
REMODEL_CLONEQUALIFIER_SPEC(&&)

// Unknown-size array qualifiers
REMODEL_CLONEQUALIFIER_SPEC([]);

#undef REMODEL_CLONEQUALIFIER_SPEC

// Known-size array qualifiers
template<typename DstT, typename SrcT, std::size_t N>
struct CloneQualifierImpl<DstT, SrcT[N]>
{
    using Type = DstT[N];
};

} // namespace internal

/**
 * @brief   Clones the first layer of qualifiers from one type to another.
 * @tparam  DstT    The destination type the qualifiers are applied to.
 * @tparam  SrcT    The source type the qualifiers are cloned from.
 */
template<typename DstT, typename SrcT>
using CloneQualifier = typename internal::CloneQualifierImpl<DstT, SrcT>::Type;

// ---------------------------------------------------------------------------------------------- //
// [ApplyQualifierStack]                                                                          //
// ---------------------------------------------------------------------------------------------- //

namespace internal
{
    
// Implementation capturing on an exhausted stack.
template<typename DstT, typename QualifierStackT, typename=void>
struct ApplyQualifierStackImpl
{
    using Type = DstT;
};

// Implementation capturing if there are elements left to process
template<typename DstT, typename QualifierStackT>
struct ApplyQualifierStackImpl<DstT, QualifierStackT, std::enable_if_t<!QualifierStackT::kEmpty>>
    : ApplyQualifierStackImpl<
        CloneQualifier<DstT, typename QualifierStackT::Top>,
        typename QualifierStackT::Pop
    >
{};

} // namespace internal

/**
 * @brief   Applies a stack of qualifiers to a type.
 * @tparam  DstT            The destination type the qualifiers are applied to.
 * @tparam  BaseTypeT       Source type for CV-qualifiers on the target base-type.
 * @tparam  QualifierStackT A stack of additional qualifiers applied to the type.
 */
template<typename DstT, typename BaseTypeT, typename QualifierStackT>
using ApplyQualifierStack 
    = typename internal::ApplyQualifierStackImpl<
        CloneQualifier<
            std::remove_const_t<
                std::remove_volatile_t<
                    typename AnalyzeQualifiers<DstT>::BaseType
                    >
                >,
            BaseTypeT
        >,
    QualifierStackT
>::Type;

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
// [Optional] + helper classes                                                                    //
// ============================================================================================== //

/**
 * @brief   Marker type allowing implicit creation of empty optionals.
 * @see     Optional
 */
static const struct EmptyT { EmptyT() {} } kEmpty;

/**
 * @brief   Marker type to enforce in-place creation.
 * @see     Optional
 */
static const struct InPlaceT { InPlaceT() {} } kInPlace;

/**
 * @brief   Terminates the program after a fatal error occurred.
 * @param   why Short description of the error (currently unused).
 */
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
    /**
     * @brief   Type of the value.
     */
    using ValueType = T;

    /**
     * @brief   Default constructor, creats an empty optional.
     */
    OptionalImplBase() : OptionalImplBase{kEmpty} {}

    /**
     * @brief   Constructor creating an empty optional.
     * @param   empty @c kEmpty constant.
     */
    OptionalImplBase(EmptyT) : m_hasValue{false} {}

    /**
     * @brief   Constructor.
     * @tparam  ArgsT   The argument types used for in-place construction.
     * @param   inplace @c kInPlace constant.
     * @param   args    Arguments passed to the constructor for in-olace construction.
     */
    template<typename... ArgsT>
    OptionalImplBase(InPlaceT, ArgsT... args)
        : m_hasValue{true}
    {
        new (ptr()) T{args...};
    }

    /**
     * @brief   Destructor.
     */
    ~OptionalImplBase()
    {
        destroyValue();
    }
public: // Observers
    /**
     * @brief   Query if this object has a value assigned.
     * @return  true if the optional has a value, false if not.
     */
    bool hasValue() const { return m_hasValue; }

    /**
     * @brief   Convenience operator equivalent to @hasValue.
     * @copydetails hasValue
     */
    operator bool () const { return hasValue(); }

    /**
     * @brief   Gets the value.
     * @return  The value.
     * @warning When called on optional without value, the behaviour is undefined.
     */
    T& value()
    {
        if (!m_hasValue) fatalExit("tried to retrieve value of Optional without value");
        return *ptr();
    }

    /**
     * @brief   Releases the value from the optional, leaving behind an empty optional.
     * @return  The value.
     * @warning When called on optional without value, the behaviour is undefined.
     */
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
    OptionalImpl() : OptionalImplBase<T>(kEmpty) /* MSVC12 requires parantheses here */ {}         \
    OptionalImpl(EmptyT) : OptionalImplBase<T>(kEmpty) /* ^ */ {}                                  \
    template<typename... ArgsT>                                                                    \
    OptionalImpl(InPlaceT, ArgsT... args) : OptionalImplBase<T>(kInPlace, args...) /* ^ */ {}

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
struct OptionalImpl final
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

/**
 * @brief   Class representing "nullable" objects which may not (yet) have a value assigned.
 * @tparam  The value type of the optional.
 */
template<typename T>
using Optional = internal::OptionalImpl<T>;

// ============================================================================================== //

} // namespace utils
} // namespace remodel

#endif // _REMODEL_UTILS_HPP_