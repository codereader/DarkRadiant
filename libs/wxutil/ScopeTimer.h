#pragma once

#include "itextstream.h"
#include <wx/stopwatch.h>
#include <fmt/format.h>

namespace wxutil
{

class ScopeTimer
{
private:
    wxStopWatch _timer;
    std::string _message;

public:
    ScopeTimer(const std::string& message) : 
        _message(message)
    {
        _timer.Start();
    }

    ~ScopeTimer()
    {
        double elapsed_time = _timer.Time() / 1000.f;

        rMessage() << _message << " timer: " << fmt::format("{0:5.2f}", elapsed_time) 
            << " second(s) elapsed" << std::endl;
    }
};

}
