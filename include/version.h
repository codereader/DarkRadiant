#ifdef HAVE_CONFIG_H
#include <config.h>
#define RADIANT_VERSION PACKAGE_VERSION
#else
#define RADIANT_VERSION "1.8.1"
#endif

#define RADIANT_APPNAME "DarkRadiant"
#define RADIANT_BLANK " "

#if defined(_M_X64) || defined(__amd64__) || defined(_WIN64)
	#define RADIANT_PLATFORM "x64"
#else
	#define RADIANT_PLATFORM "x86"
#endif

#include <string>

inline std::string RADIANT_APPNAME_FULL()
{
   return std::string(RADIANT_APPNAME) + " "
          + std::string(RADIANT_VERSION) + " "
          + std::string(RADIANT_PLATFORM) + " ";
}
