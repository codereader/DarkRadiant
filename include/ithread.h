#pragma once

#include <functional>

/**
 * \brief
 * Interface to the threading manager.
 *
 * The ThreadManager wraps a thread pool which is owned and managed by the
 * main Radiant application.
 */
class ThreadManager
{
public:

    /// Execute the given function in a separate thread
	/// Returns the thread id, which can be used to query the state
	virtual std::size_t execute(std::function<void()> func) = 0;

	// Returns true if the given thread is still running
	virtual bool threadIsRunning(std::size_t threadId) = 0;
};
