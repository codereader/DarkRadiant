#pragma once

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <functional>
#include <thread>
#include <memory>

namespace util
{

/**
 * Timer class able to fire a callback in regular intervals.
 * Pass the callback function to be invoked on interval reach to the constructor.
 *
 * Use the start() method to enable the timer (which is disabled after construction)
 * Use the stop() method to disable the timer.
 */
class Timer
{
private:
	std::condition_variable _condition;
	std::mutex _lock;
	std::unique_ptr<std::thread> _worker;
	std::shared_ptr<bool> _cancellationToken;
	std::size_t _intervalMsecs;
	std::function<void()> _intervalReached;

public:
	Timer(std::size_t intervalMsecs, const std::function<void()>& intervalReached) :
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

		if (_intervalMsecs == 0)
		{
			throw std::runtime_error("Cannot start timer interval set to 0");
		}

		// Allocate a new cancellation token and remember it
		// The new thread will keep a reference to this exact bool instance
		_cancellationToken = std::make_shared<bool>(false);

		// Spawn the thread, passing the reference to the token
		_worker.reset(new std::thread(std::bind(&Timer::run, this, _cancellationToken)));
	}

	void stop()
	{
		if (!_worker)
		{
			return;
		}

		// If we have a running worker, we also need a token for it
		assert(_cancellationToken);

		{
			// Set the cancel signal of the existing thread
			// Accessing the shared bool instance requires a lock
			std::lock_guard<std::mutex> lock(_lock);
			*_cancellationToken = true;
		}

		// In case we got here from the thread itself
		// (client callback is stopping the timer)
		// don't attempt to join our own thread
		if (_worker->get_id() == std::this_thread::get_id())
		{
			// Just detach the thread, the cancel token is already set to true
			_worker->detach();
		}
		else
		{
			// Send the signal to wake up the running thread to check the token
			_condition.notify_one();

			// Wait for the existing worker to stop
			_worker->join();
		}

		// Clear out the references, we're ready to start another worker
		_worker.reset();
		_cancellationToken.reset();
	}

private:
	void run(std::shared_ptr<bool> token)
	{
		// Store the reference to the cancellation token locally
		std::shared_ptr<bool> cancellationToken(token);

		while (true)
		{
			// Acquire the lock to access the token
			std::unique_lock<std::mutex> lock(_lock);

			// Check cancel flag before and after the wait state
			if (*cancellationToken) return;

			// Wait for the interval or until the cancel flag is hit
			// this will unlock the mutex
			_condition.wait_for(lock, std::chrono::milliseconds(_intervalMsecs), [&]
			{
				return *cancellationToken;
			});

			if (*cancellationToken) return; // on cancel signal, break this loop

			// Unlock the mutex such that the callback is able
			// to acquire it (e.g. to set the cancel signal)
			lock.unlock();

			// Interval reached
			_intervalReached();
		}
	}
};

}
