#pragma once

#include "icommandsystem.h"

namespace ui
{

constexpr const char* const RKEY_SCRIPT_REFERENCE_URL = "user/ui/scriptReferenceUrl";

/// Utility class to launch a browser with various documentation
class Documentation
{
public:

    /// Show the user guide online
    static void showUserGuide(const cmd::ArgumentList&);

    // Open the TDM forums
    static void OpenForumUrl(const cmd::ArgumentList&);

    // Navigate to the DR script reference
    static void OpenScriptReference(const cmd::ArgumentList&);

    /// Show the locally-installed user guide
    static void showOfflineUserGuide(const cmd::ArgumentList&);
};

}
