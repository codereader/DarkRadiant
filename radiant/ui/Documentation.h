#pragma once

#include "icommandsystem.h"

namespace ui
{

/// Utility class to launch a browser with various documentation
class Documentation
{
public:

    /// Show the user guide
    static void showUserGuide(const cmd::ArgumentList&);
};

}
