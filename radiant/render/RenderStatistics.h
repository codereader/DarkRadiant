#pragma once

#include <wx/stopwatch.h>
#include "string/string.h"

namespace render
{

/// Storage class for per-frame render statistics
class RenderStatistics
{
    // Timer for measuring render time
    wxStopWatch _timer;

public:

    /// Return the constructed string for display
    std::string getStatString()
    {
        return "msec: " + std::to_string(_timer.Time());
    }

    /// Reset statistics at the beginning of a frame render
    void resetStats()
    {
        _timer.Start();
    }
};

} // namespace render
