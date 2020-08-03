#pragma once

#include <condition_variable>
#include <mutex>
#include <functional>
#include <thread>

namespace util
{

class Timer
{
private:
	bool _cancel;
	std::condition_variable _condition;
	std::mutex _lock;
	std::unique_ptr<std::thread> _worker;
	std::size_t _intervalMsecs;
	std::function<void()> _intervalReached;

public:
	Timer(std::size_t intervalMsecs, const std::function<void()>& intervalReached) :
		_cancel(false),
		_intervalMsecs(intervalMsecs),
		_intervalReached(intervalReached)
	{
		if (_intervalMsecs == 0)
		{
			throw std::runtime_error("Cannot set timer interval to 0 msecs");
		}
	}

	~Timer()
	{
		stop();
	}

	// Start the timer or restart it if already running. 
	// Leave the intervalMsecs parameter at 0 to use the currently active interval
	void start(std::size_t intervalMsecs = 0)
	{
		stop();

		if (intervalMsecs > 0)
		{
			_intervalMsecs = intervalMsecs;
		}

		// At this point, no thread is running, so no need to lock this
		_cancel = false;

		if (_intervalMsecs == 0)
		{
			throw std::runtime_error("Cannot start timer interval set to 0");
		}

		_worker.reset(new std::thread(std::bind(&Timer::run, this)));
	}

	void stop()
	{
		if (!_worker)
		{
			return;
		}

		{
			// Wait for the existing worker to stop
			std::lock_guard<std::mutex> lock(_lock);
			_cancel = true;
		}

		_condition.notify_one();

		_worker->join();
		_worker.reset();
	}

private:
	void run()
	{
		while (true)
		{
			// Acquire the lock
			std::unique_lock<std::mutex> lock(_lock);

			// Wait for the interval or until the cancel flag is hit
			// this will unlock the mutex
			_condition.wait_for(lock, std::chrono::milliseconds(_intervalMsecs), [this]
			{
				return _cancel;
			});

			if (_cancel)
			{
				return;
			}

			// Unlock the mutex such that the callback is able
			// to acquire it (e.g. to cancel the thread)
			lock.unlock();

			// Interval reached
			_intervalReached();
		}
	}
};

}
