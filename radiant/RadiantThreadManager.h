#pragma once

#include "ithread.h"
#include <memory>
#include <map>
#include <vector>

namespace radiant
{

/// ThreadManager implementation class
class RadiantThreadManager : 
	public ThreadManager
{
	class DetachedThread;
	typedef std::shared_ptr<DetachedThread> DetachedThreadPtr;

	typedef std::map<std::size_t, DetachedThreadPtr> ThreadMap;
	ThreadMap _threads;

	std::vector<DetachedThreadPtr> _finishedThreads;

public:
	~RadiantThreadManager();

    // ThreadManager implementation
	std::size_t execute(std::function<void()>);
	bool threadIsRunning(std::size_t threadId);

	// Called by the worker threads
	void onThreadFinished(std::size_t threadId);

private:
	std::size_t getFreeThreadId();
};

}
