#pragma once

#include <chrono>

namespace util
{

/**
 * Helper class wrapping a std::chrono::steady_clock to measure
 * time intervals in seconds.
 * Use the getSecondsPassed() method to measure the time passed
 * since the last call to restart(). If no call to restart() was
 * made, it will return the time passed since construction.
 */
class StopWatch
{
private:
	std::chrono::steady_clock _clock;

	std::chrono::steady_clock::time_point _start;

public:
	StopWatch()
	{
		restart();
	}

	// Restarts the timer, resetting the time passed to 0
	void restart()
	{
		_start = _clock.now();
	}

	// Returns the seconds passed since the last call to restart()
	// If restart() has never been called, this is the time since construction
	double getSecondsPassed() const
	{
		auto endTime = _clock.now();

		return std::chrono::duration_cast<std::chrono::seconds>(endTime - _start).count();
	}
};

}
