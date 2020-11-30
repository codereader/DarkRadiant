#pragma once

#include <fmt/format.h>

namespace os
{

// Formats the given number in bytes/kB/MB/GB
inline std::string getFormattedFileSize(std::size_t size)
{
    if (size > 1024 * 1024 * 1024)
    {
        return fmt::format("{0:0.2f} GB", (static_cast<double>(size) / (1024 * 1024 * 1024)));
    }
    else if (size > 1024 * 1024)
    {
        return fmt::format("{0:0.1f} MB", (static_cast<double>(size) / (1024 * 1024)));
    }
    else if (size > 1024)
    {
        return fmt::format("{0:0.0f} kB", (static_cast<double>(size) / 1024));
    }
    else
    {
        return fmt::format("{0:d} bytes", size);
    }
}

}
