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

    // Time for the render front-end only
    long _feTime = 0;

public:

    /// Return the constructed string for display
    std::string getStatString()
    {
        // Calculate times for render front-end and back-end
        long totTime = _timer.Time();
        long beTime = totTime - _feTime;

        return " | f/e: " + std::to_string(_feTime) + " ms"
             + " | b/e: " + std::to_string(beTime) + " ms"
             + " | tot: " + std::to_string(totTime) + " ms"
             + " | fps: " + (totTime > 0 ? std::to_string(1000 / totTime) : "-");
    }

    /// Mark the front-end render stage as completed, storing the time internally
    void frontEndComplete()
    {
        _feTime = _timer.Time();
    }

    /// Reset statistics at the beginning of a frame render
    void resetStats()
    {
        _feTime = 0;
        _timer.Start();
    }
};

} // namespace render
