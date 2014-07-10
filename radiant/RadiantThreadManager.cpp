#include "RadiantThreadManager.h"

#include <wx/thread.h>
#include <algorithm>

namespace radiant
{

class RadiantThreadManager::DetachedThread : 
	public wxThread
{
private:
	boost::function<void()> _func;

public:
	DetachedThread(const boost::function<void()>& func) :
		wxThread(wxTHREAD_JOINABLE),
		_func(func)
	{}

	ExitCode Entry()
	{
		_func();
		return static_cast<ExitCode>(0);
	}
};

RadiantThreadManager::~RadiantThreadManager()
{
	std::for_each(_threads.begin(), _threads.end(), [] (const DetachedThreadPtr& thread)
	{
		if (thread->IsRunning())
		{
			thread->Delete();
		}
	});

	std::for_each(_threads.begin(), _threads.end(), [] (const DetachedThreadPtr& thread)
	{
		if (thread->IsRunning())
		{
			thread->Wait();
		}
	});

	_threads.clear();
}

void RadiantThreadManager::execute(boost::function<void()> func)
{
	DetachedThreadPtr thread(new DetachedThread(func));
	_threads.push_back(thread);

	thread->Run();
}

}
