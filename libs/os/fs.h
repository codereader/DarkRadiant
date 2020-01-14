/**
 * \file
 * Helper header for std::filesystem feature set. Includes the relevant headers
 * and defines the common "fs" namespace alias. 
 * If the compiler library supports the C++17 feature std::filesystem, it includes
 * the corresponding headers, or fall back to std::experimental::filesystem::v1.
 * All other compilers will refer to the headers provided by boost::filesystem.
 */
#pragma once

// We need the HAVE_* symbols created by the configure script
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// If C++17 <filesystem> is available, use that one
// e.g. Visual Studio 2019 provides the regular C++17 header
#if defined(HAVE_STD_FILESYSTEM) || defined(__cpp_lib_filesystem) || _MSC_VER >= 1920

#include <filesystem>
namespace fs = std::filesystem;
#define DR_USE_STD_FILESYSTEM

// For older compilers C++17 is still in draft state, but some compilers
// provide the features through the std::experimental namespace.
// In Linux, the configure script will check for the header and define the
// HAVE_EXPERIMENTAL_FILESYSTEM symbol for us.
#elif _MSC_VER >= 1900 || defined(HAVE_EXPERIMENTAL_FILESYSTEM)

// Visual Studio 2015/2017 and GCC 5.3+ supply the experimental/filesystem header
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem::v1;
#define DR_USE_STD_FILESYSTEM

#else

// All other compilers will use the boost headers
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#define DR_USE_BOOST_FILESYSTEM

namespace fs = boost::filesystem;

#endif

#include "string/predicate.h"

namespace os
{
    /// Overload of standardPathWithSlash that accepts a fs::path
    inline std::string standardPathWithSlash(const fs::path& p)
    {
        std::string genString = p.generic_string();

        // Just add slash if needed, we don't need to convert intermediate
        // slashes since string_from_path will already have done that.
		if (!string::ends_with(genString, "/"))
        {
			genString += "/";
		}

        return genString;
    }

	// Wrapper method to return the depth of a recursive iterator,
	// supporting std::experimental::filesystem as well as boost::filesystem
	inline int getDepth(fs::recursive_directory_iterator& it)
	{
#ifdef DR_USE_STD_FILESYSTEM
		return it.depth();
#else
		return it.level();
#endif
	}

	// Wrapper method to call no_push / disable_recursion_pending on iterators
	// Since the C++17 call has been named differently than the boost one, here we go
	inline void disableRecursionPending(fs::recursive_directory_iterator& it)
	{
#ifdef DR_USE_STD_FILESYSTEM
		it.disable_recursion_pending();
#else
		it.no_push();
#endif
	}

	// Replaces the extension of the given filename with newExt
	inline std::string replaceExtension(const std::string& input, const std::string& newExt)
	{
#ifdef DR_USE_STD_FILESYSTEM
		return fs::path(input).replace_extension(newExt).string();
#else
		return fs::change_extension(input, newExt).string();
#endif
	}
}
