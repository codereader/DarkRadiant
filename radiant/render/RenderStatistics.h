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

    // Count of lights
    int _lights = 0;

public:

    /// Return the constructed string for display
    std::string getStatString()
    {
        long msec = _timer.Time();
        return "lights: " + std::to_string(_lights)
             + " | msec: " + std::to_string(msec)
             + " | fps: " + (msec > 0 ? std::to_string(1000 / msec) : "-");
    }

    /// Increment the light count
    void addLight()
    {
        ++_lights;
    }

    /// Reset statistics at the beginning of a frame render
    void resetStats()
    {
        _lights = 0;

        _timer.Start();
    }
};

} // namespace render
