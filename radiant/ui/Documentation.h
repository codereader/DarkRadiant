#pragma once

#include "icommandsystem.h"

namespace ui
{

/// Utility class to launch a browser with various documentation
class Documentation
{
public:

    /// Show the user guide online
    static void showUserGuide(const cmd::ArgumentList&);

    // Open the TDM forums
    static void OpenForumUrl(const cmd::ArgumentList&);

    /// Show the locally-installed user guide
    static void showOfflineUserGuide(const cmd::ArgumentList&);
};

}
