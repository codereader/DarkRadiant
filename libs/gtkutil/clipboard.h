#pragma once

#include <string>

namespace wxutil
{
    /// Copy the given string to the system clipboard
    void copyToClipboard(const std::string& str);

    /// Return the contents of the clipboard as a string
    std::string pasteFromClipboard();
}
