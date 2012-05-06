#pragma once

#include <glibmm.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

/**
 * \brief
 * Interface to the threading manager.
 *
 * The ThreadManager wraps a Glib::ThreadPool which is owned and managed by the
 * main Radiant application.
 */
class ThreadManager
{
public:

    /// Execute the given function in a separate thread
    virtual void execute(boost::function<void()> func) const = 0;
};
