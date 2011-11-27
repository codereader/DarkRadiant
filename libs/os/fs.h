/**
 * \file
 * Helper header for boost::filesystem. Includes the relevant Boost headers,
 * defines the common "fs" namespace alias, and includes some helper functions
 * to deal with the transition from Filesystem V2 to V3.
 */
#pragma once

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace os
{
    /// Return the string filename of a path object
    inline std::string get_filename(const fs::path& p)
    {
#if BOOST_VERSION < 104600
        return p.filename();
#else
        return p.filename().string();
#endif
    }

    /// Return a string for the given path in platform-independent format
    inline std::string get_generic_string(const fs::path& p)
    {
#if BOOST_VERSION < 104600
        return p.string(); // in contrast to file_string() (native format)
#else
        return p.generic_string(); // in this case string() is native format
#endif
    }

}
