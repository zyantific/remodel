#ifndef _REMODEL_PLATFORM_HPP_
#define _REMODEL_PLATFORM_HPP_

#include "config.hpp"

#ifdef REMODEL_WIN32
#   include <Windows.h>
#elif REMODEL_POSIX
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
#   ifdef REMODEL_WIN32
        return GetModuleHandleA(moduleName);
#   elif REMODEL_POSIX
        return dlopen(moduleHandle, 0);
#   else
#       error "Platform not supported"
#   endif
}

// ---------------------------------------------------------------------------------------------- //

}
} // namespace remodel

#endif // _REMODEL_PLATFORM_HPP_
