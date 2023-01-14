#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#define RADIANT_VERSION PACKAGE_VERSION
#else
#define RADIANT_VERSION "3.7.0"
#endif

#define RADIANT_APPNAME "DarkRadiant"
#define RADIANT_BLANK " "

#if defined(_M_X64) || defined(__amd64__) || defined(_WIN64)

// 64 bit architecture names (according to platform convention)
#if defined(__linux__)
    #define RADIANT_PLATFORM "amd64"
#else
	#define RADIANT_PLATFORM "x64"
#endif

#else

// 32 bit architecture names (according to platform convention)
#if defined(__linux)
    #define RADIANT_PLATFORM "i386"
#else
	#define RADIANT_PLATFORM "x86"
#endif

#endif

#include <string>

inline std::string RADIANT_APPNAME_FULL()
{
   return std::string(RADIANT_APPNAME) + " "
          + std::string(RADIANT_VERSION) + " "
          + std::string(RADIANT_PLATFORM) + " ";
}
