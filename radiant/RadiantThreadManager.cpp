#include "RadiantThreadManager.h"

namespace radiant
{

namespace
{
    void runFuncInThread(boost::function<void()> func)
    {
        func();
    }
}

void RadiantThreadManager::execute(boost::function<void()> func) const
{
    // Use adapter function to call our boost::function in a thread (since
    // ThreadPool requires a sigc::slot).
    _pool.push(sigc::bind(sigc::ptr_fun(&runFuncInThread), func));
}

}
