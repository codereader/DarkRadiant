#pragma once

#include <chrono>
#include <functional>
#include <thread>

namespace test
{

namespace algorithm
{

// Waits until the given condition is met, but no longer than the given amount of milliseconds
// Returns true if the stop condition has been met before the timeout, false otherwise
inline bool waitUntil(std::function<bool()> stopCondition, int maxMillisecondsToWait = 30000)
{
    using namespace std::chrono_literals;
    auto startTime = std::chrono::steady_clock::now();

    while (!stopCondition())
    {
        std::this_thread::sleep_for(10ms);

        auto timeSinceStart = std::chrono::steady_clock::now() - startTime;
        auto millisecondsPassed = std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceStart);

        if (millisecondsPassed.count() > maxMillisecondsToWait)
        {
            return false;
        }
    }

    return true;
}

}

}
