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

#if defined(_MSC_VER)
#   define REMODEL_MSVC
#elif defined(__GNUC__) || defined(__GNUG__)
#   define REMODEL_GNUC
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


#endif // _REMODEL_CONFIG_HPP_
