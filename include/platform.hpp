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

/**
 * @brief   Obtain the handle of a loaded module (DLL, dynlib, SO, ...).
 * @param   moduleName  Name of the module or @c nullptr for the main-module (executable).
 * @return  @c nullptr if the module is not loaded, else a pointer to the module's first byte.
 */
inline void* obtainModuleHandle(const char* moduleName)
{
#   if defined(REMODEL_WIN32)
        return GetModuleHandleA(moduleName);
#   elif defined(REMODEL_POSIX)
        return dlopen(moduleName, RTLD_NOLOAD);
#   else
#       error "Platform not supported"
#   endif
}

// ---------------------------------------------------------------------------------------------- //

}
} // namespace remodel

#endif // _REMODEL_PLATFORM_HPP_
