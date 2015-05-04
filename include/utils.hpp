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

} // namespace Utils
} // namespace Remodel

#endif // _REMODEL_UTILS_HPP_