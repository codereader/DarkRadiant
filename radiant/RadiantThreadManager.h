#pragma once

#include "ithread.h"
#include <memory>
#include <list>

namespace radiant
{

/// ThreadManager implementation class
class RadiantThreadManager : 
	public ThreadManager
{
	class DetachedThread;
	typedef std::shared_ptr<DetachedThread> DetachedThreadPtr;

	std::list<DetachedThreadPtr> _threads;

public:
	~RadiantThreadManager();

    // ThreadManager implementation
    void execute(boost::function<void()>);
};

}
