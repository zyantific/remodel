#ifndef _REMODEL_CONFIG_HPP_
#define _REMODEL_CONFIG_HPP_

#define REMODEL_NOTHROW throw()

#ifdef _MSC_VER
#   define REMODEL_MSVC
#elif __GNUC__
#   define REMODEL_GNUC
#endif

#endif // _REMODEL_CONFIG_HPP_