/**
 * This file is part of the remodel library (zyantific.com).
 * 
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Joel Höner (athre0z)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, 
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _REMODEL_PLATFORM_HPP_
#define _REMODEL_PLATFORM_HPP_

/**     
 * @file
 * @brief Contains wrappers for platform-specific functions.
 */

#include "zycore/Config.hpp"

#if defined(ZYCORE_WIN32)
#   include <Windows.h>
#elif defined(ZYCORE_POSIX)
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
 * @brief   Obtain the handle of a loaded module (DLL, dylib, SO, ...).
 * @param   moduleName  Name of the module or @c nullptr for the main-module.
 * @return  @c nullptr if the module is not loaded, else a pointer to the module's first byte.
 */
inline void* obtainModuleHandle(const char* moduleName)
{
#   if defined(ZYCORE_WIN32)
        return GetModuleHandleA(moduleName);
#   elif defined(ZYCORE_POSIX)
        return dlopen(moduleName, RTLD_NOLOAD);
#   else
#       error "Platform not supported"
#   endif
}

// ---------------------------------------------------------------------------------------------- //

}
} // namespace remodel

#endif // _REMODEL_PLATFORM_HPP_
