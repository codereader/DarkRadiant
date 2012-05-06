#pragma once

#include "ithread.h"

namespace radiant
{

/// ThreadManager implementation class
class RadiantThreadManager : public ThreadManager
{
    // The threadpool we use for executing jobs
    mutable Glib::ThreadPool _pool;

public:

    // ThreadManager implementation
    void execute(boost::function<void()>) const;
};

}
