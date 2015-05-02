#ifndef _REMODEL_UTILS_HPP_
#define _REMODEL_UTILS_HPP_

#if defined(_DEBUG) && defined(_WIN32)
#   include <Windows.h>
#endif

#include <type_traits>

namespace Remodel
{
namespace Utils
{;

// ============================================================================================== //
// TMP tools                                                                                      //
// ============================================================================================== //

/**  
 * @brief   Black-box constants forcing compiler to perform lazy evaluation on constants.
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

// ============================================================================================== //
// Misc                                                                                           //
// ============================================================================================== //

inline void fatalError(const char *what)
{
#   if defined(_DEBUG) && defined(_WIN32)
        MessageBoxA(nullptr, what, "remodel fatal", MB_ICONERROR);
#   endif
    std::terminate();
}

// ============================================================================================== //

} // namespace Utils
} // namespace Remodel

#endif // _REMODEL_UTILS_HPP_