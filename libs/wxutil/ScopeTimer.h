#pragma once

#include "itextstream.h"
#include <wx/stopwatch.h>
#include <boost/format.hpp>

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

        rMessage() << _message << " timer: " << (boost::format("%5.2lf") % elapsed_time).str() 
            << " second(s) elapsed" << std::endl;
    }
};

}
