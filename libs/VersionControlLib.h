#pragma once

#include <string>
#include <regex>
#include <fmt/format.h>

namespace vcs
{

namespace detail
{
    constexpr const char* const UriPattern = "^(\\w+)://(\\w+)/(.+)$";
    constexpr std::size_t PrefixMatchIndex = 1;
    constexpr std::size_t RevisionMatchIndex = 2;
    constexpr std::size_t FilePathMatchIndex = 3;

    constexpr const char* const UriFormat = "{0}://{1}/{2}";

    inline std::string getVcsPatternMatch(const std::string& uri, std::size_t index)
    {
        std::smatch results;
        return std::regex_match(uri, results, std::regex(UriPattern)) ? results[index].str() : std::string();
    }

}

inline bool pathIsVcsUri(const std::string& path)
{
    return std::regex_match(path, std::regex(detail::UriPattern));
}

inline std::string getVcsPrefix(const std::string& uri)
{
    return detail::getVcsPatternMatch(uri, detail::PrefixMatchIndex);
}

inline std::string getVcsRevision(const std::string& uri)
{
    return detail::getVcsPatternMatch(uri, detail::RevisionMatchIndex);
}

inline std::string getVcsFilePath(const std::string& uri)
{
    return detail::getVcsPatternMatch(uri, detail::FilePathMatchIndex);
}

inline std::string constructVcsFileUri(const std::string& prefix, const std::string& revision, const std::string& path)
{
    return fmt::format(detail::UriFormat, prefix, revision, path);
}

}
