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

#ifndef _REMODEL_CONFIG_HPP_
#define _REMODEL_CONFIG_HPP_

// ============================================================================================== //
// Compiler detection                                                                             //
// ============================================================================================== //

#if defined(__clang__)
#   define REMODEL_CLANG
#   define REMODEL_GNUC
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#   define REMODEL_ICC
#elif defined(__GNUC__) || defined(__GNUG__)
#   define REMODEL_GCC
#   define REMODEL_GNUC
#elif defined(_MSC_VER)
#   define REMODEL_MSVC
#else
#   define REMODEL_UNKNOWN_COMPILER
#endif

// ============================================================================================== //
// Platform detection                                                                             //
// ============================================================================================== //

#if defined(_WIN32)
#   define REMODEL_WIN32
#elif defined(__APPLE__)
#   define REMODEL_APPLE
#   define REMODEL_POSIX
#elif defined(__linux)
#   define REMODEL_LINUX
#   define REMODEL_POSIX
#elif defined(__unix)
#   define REMODEL_UNIX
#   define REMODEL_POSIX
#elif defined(__posix)
#   define REMODEL_POSIX
#else
#   error "Unsupported platform detected"
#endif

// ============================================================================================== //
// Verification of assumptions                                                                    //
// ============================================================================================== //

// Data pointer size == code pointer size?
static_assert(sizeof(void(*)()) == sizeof(void*), "unsupported platform");

// ============================================================================================== //
// Workarounds for compiler bugs                                                                  //
// ============================================================================================== //

#define REMODEL_COMMA ,

// https://llvm.org/bugs/show_bug.cgi?id=23554
#ifdef REMODEL_CLANG
#   define REMODEL_DECLTYPE_AUTO_WA(expr) decltype(auto)
#else
#   define REMODEL_DECLTYPE_AUTO_WA(expr) decltype(expr)
#endif

// ============================================================================================== //


#endif // _REMODEL_CONFIG_HPP_
