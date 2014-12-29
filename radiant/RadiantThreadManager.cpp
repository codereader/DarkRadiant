#include "RadiantThreadManager.h"

#include <wx/thread.h>
#include <algorithm>
#include <stdexcept>

namespace radiant
{

class RadiantThreadManager::DetachedThread : 
	public wxThread
{
private:
	std::size_t _threadId;
	std::function<void()> _func;
	RadiantThreadManager& _owner;

public:
	DetachedThread(std::size_t threadId, std::function<void()>& func, RadiantThreadManager& owner) :
		wxThread(wxTHREAD_JOINABLE),
		_threadId(threadId),
		_func(func),
		_owner(owner)
	{}

	void OnExit()
	{
		_owner.onThreadFinished(_threadId);
	}

	ExitCode Entry()
	{
		_func();

		return static_cast<ExitCode>(0);
	}
};

RadiantThreadManager::~RadiantThreadManager()
{
	std::for_each(_threads.begin(), _threads.end(), [] (const ThreadMap::value_type& pair)
	{
		if (pair.second->IsRunning())
		{
			pair.second->Delete();
		}
	});

	std::for_each(_threads.begin(), _threads.end(), [](const ThreadMap::value_type& pair)
	{
		if (pair.second->IsRunning())
		{
			pair.second->Wait();
		}
	});

	_threads.clear();
}

std::size_t RadiantThreadManager::execute(std::function<void()> func)
{
	std::size_t threadId = getFreeThreadId();

	_threads[threadId] = DetachedThreadPtr(new DetachedThread(threadId, func, *this));
	_threads[threadId]->Run();

	return threadId;
}

bool RadiantThreadManager::threadIsRunning(std::size_t threadId)
{
	ThreadMap::const_iterator found = _threads.find(threadId);

	if (found == _threads.end()) return false;

	return found->second->IsRunning();
}

void RadiantThreadManager::onThreadFinished(std::size_t threadId)
{
	ThreadMap::iterator found = _threads.find(threadId);

	assert(found != _threads.end());

	// Don't delete the thread yet, it's stil used by wxWidgets
	_finishedThreads.push_back(found->second);
	_threads.erase(found);
}

std::size_t RadiantThreadManager::getFreeThreadId()
{
	_finishedThreads.clear();

	for (std::size_t i = 1; i < std::numeric_limits<std::size_t>::max(); ++i)
	{
		if (_threads.find(i) == _threads.end())
		{
			return i;
		}
	}

	throw std::runtime_error("No more free thread IDs.");
}

}
