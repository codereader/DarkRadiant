#pragma once

#include <boost/function.hpp>

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
    virtual void execute(boost::function<void()> func) = 0;
};
