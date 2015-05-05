#ifndef _REMODEL_PLATFORM_HPP_
#define _REMODEL_PLATFORM_HPP_

#include "config.hpp"

#if defined(REMODEL_WIN32)
#   include <Windows.h>
#elif defined(REMODEL_POSIX)
#   include <dlfcn.h>
#endif

namespace remodel
{
namespace platform
{
    
// ---------------------------------------------------------------------------------------------- //
// [obtainModuleHandle]                                                                           //
// ---------------------------------------------------------------------------------------------- //

inline void* obtainModuleHandle(const char* moduleName)
{
#   if defined(REMODEL_WIN32)
        return GetModuleHandleA(moduleName);
#   elif defined(REMODEL_POSIX)
        return dlopen(moduleName, 0);
#   else
#       error "Platform not supported"
#   endif
}

// ---------------------------------------------------------------------------------------------- //

}
} // namespace remodel

#endif // _REMODEL_PLATFORM_HPP_
