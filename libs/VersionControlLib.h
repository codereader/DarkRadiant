#pragma once

#include <string>
#include <regex>

namespace vcs
{

constexpr const char* const UriPattern = "^\\(w+)://(\\w+)/(.+)";

inline bool pathIsVcsUri(const std::string& path)
{
    return std::regex_match(path, std::regex(UriPattern));
}

}
