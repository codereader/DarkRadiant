#pragma once

#include <glibmm/ustring.h>

namespace gtkutil
{
    /// Copy the given string to the system clipboard
    void copyToClipboard(const Glib::ustring& str);

    /// Return the contents of the clipboard as a string
    Glib::ustring pasteFromClipboard();
}
