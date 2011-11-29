/**
 * \file
 * Helper header for boost::filesystem. Includes the relevant Boost headers,
 * defines the common "fs" namespace alias, and includes some helper functions
 * to deal with the transition from Filesystem V2 to V3.
 */
#pragma once

#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace fs = boost::filesystem;

namespace os
{
    /// Return the string filename of a path object
    inline std::string filename_from_path(const fs::path& p)
    {
#if BOOST_VERSION < 104600
        return p.filename();
#else
        return p.filename().string();
#endif
    }

    /// Return a string for the given path in platform-independent format
    inline std::string string_from_path(const fs::path& p)
    {
#if BOOST_VERSION < 104600
        return p.string(); // in contrast to file_string() (native format)
#else
        return p.generic_string(); // in this case string() is native format
#endif
    }

    /// Overload of standardPathWithSlash that accepts a fs::path
    inline std::string standardPathWithSlash(const fs::path& p)
    {
        std::string genString = string_from_path(p);

        // Just add slash if needed, we don't need to convert intermediate
        // slashes since string_from_path will already have done that.
		if (!boost::algorithm::ends_with(genString, "/"))
        {
			genString += "/";
		}
        return genString;
    }
}
