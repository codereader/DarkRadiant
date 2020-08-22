#pragma once

#include <string>
#include <fmt/format.h>

#include "itextstream.h"
#include "StopWatch.h"

namespace util
{

class ScopeTimer
{
private:
    StopWatch _timer;
    std::string _message;

public:
    ScopeTimer(const std::string& message) :
        _message(message)
    {}

    ~ScopeTimer()
    {
        double elapsedSecs = _timer.getMilliSecondsPassed() / 1000.0;

        rMessage() << _message << " timer: " << fmt::format("{0:5.2f}", elapsedSecs)
            << " second(s) elapsed" << std::endl;
    }
};

}
