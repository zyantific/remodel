#ifndef _REMODEL_UTILS_HPP_
#define _REMODEL_UTILS_HPP_

#if defined(_DEBUG) && defined(_WIN32)
#   include <Windows.h>
#endif

namespace Remodel
{
namespace Utils
{;

// ============================================================================================== //
// Utilities                                                                                      //
// ============================================================================================== //

inline void fatalError(const char *what)
{
#   if defined(_DEBUG) && defined(_WIN32)
        MessageBoxA(NULL, what, "remodel fatal", MB_ICONERROR);
#   endif
    std::terminate();
}

// ============================================================================================== //

} // namespace Utils
} // namespace Remodel

#endif // _REMODEL_UTILS_HPP_