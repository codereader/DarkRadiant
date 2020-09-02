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
        long msec = _timer.Time();
        return "msec: " + std::to_string(msec)
             + " | fps: " + (msec > 0 ? std::to_string(1000 / msec) : "-");
    }

    /// Reset statistics at the beginning of a frame render
    void resetStats()
    {
        _timer.Start();
    }
};

} // namespace render
