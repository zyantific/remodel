#ifndef _REMODEL_CONFIG_HPP_
#define _REMODEL_CONFIG_HPP_

// ============================================================================================== //
// Compiler detection                                                                             //
// ============================================================================================== //

#ifdef _MSC_VER
#   define REMODEL_MSVC
#elif defined(__GNUC__) || defined(__GNUG__)
#   define REMODEL_GNUC
#endif

// ============================================================================================== //
// Platform detection                                                                             //
// ============================================================================================== //

#ifdef _WIN32
#   define REMODEL_WIN32
#elif __APPLE__
#   define REMODEL_APPLE
#   define REMODEL_POSIX
#elif __linux
#   define REMODEL_LINUX
#   define REMODEL_POSIX
#elif __unix
#   define REMODEL_UNIX
#   define REMODEL_POSIX
#elif __posix
#   define REMODEL_POSIX
#else
#   error "Unsupported platform detected"
#endif

// ============================================================================================== //


#endif // _REMODEL_CONFIG_HPP_
