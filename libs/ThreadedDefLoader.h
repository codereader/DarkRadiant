#pragma once

#include <future>
#include <functional>

namespace util
{

/**
 * Helper class used to asynchronically parse/load def files in a separate thread.
 *
 * The worker thread itself is ensured to be called in a thread-safe 
 * way (to prevent the worker from being invoked twice). Subsequent calls to 
 * get() or start() will not start the loader again, unless the reset() method
 * is called.
 *
 * Client code (even from multiple threads) can retrieve (and wait for) the result 
 * by calling the get() method.
 */
template <typename ReturnType>
class ThreadedDefLoader
{
    typedef std::function<ReturnType()> LoadFunction;

    LoadFunction _loadFunc;

    std::shared_future<ReturnType> _result;
    std::mutex _mutex;

    bool _loadingStarted;

public:
    ThreadedDefLoader(const LoadFunction& loadFunc) :
        _loadFunc(loadFunc),
        _loadingStarted(false)
    {}

    ~ThreadedDefLoader()
    {
        // wait for any worker thread to finish
        reset();
    }

    // Starts the loader in the background. This can be called multiple
    // times from separate threads, the worker will only launched once and 
    // cannot be started a second time unless reset() is called.
    void start()
    {
        ensureLoaderStarted();
    }

    // Ensrues that the worker thread has been started and is done processing
    // This will block and wait for the worker execution before returning to the caller.
    void ensureFinished()
    {
        get();
    }

    // Retrieve the result of the asynchronous worker function
    // This will block and wait for the result if worker has not been
    // run yet or in case it's still running
    ReturnType get()
    {
        // Make sure we already started the loader
        ensureLoaderStarted();

        // Wait for the result or return if it's already done.
        return _result.get();
    }

    // Resets the state of the loader to the state it had after construction.
    // If a background thread has been started, this will block and wait for it to finish.
    void reset()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // Wait for any running thread to finish
        if (_loadingStarted)
        {
            _loadingStarted = false;

            if (_result.valid())
            {
                _result.get();
            }

            _result = std::shared_future<ReturnType>();
        }
    }

private:
    void ensureLoaderStarted()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_loadingStarted)
        {
            _loadingStarted = true;
            _result = std::async(std::launch::async, _loadFunc);
        }
    }
};

}
