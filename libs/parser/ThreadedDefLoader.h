#pragma once

#include <future>
#include <functional>
#include <algorithm>
#include <sigc++/signal.h>
#include <vector>

namespace parser
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
public:
    using LoadFunction = std::function<ReturnType()>;
    using FinishedSignal = sigc::signal<void>;

private:
    LoadFunction _loadFunc;
    FinishedSignal _finishedSignal;

    std::shared_future<ReturnType> _result;
    std::shared_future<void> _finisher;
    std::mutex _mutex;

    bool _loadingStarted;

public:
    ThreadedDefLoader(const LoadFunction& loadFunc) :
        _loadFunc(loadFunc),
        _loadingStarted(false)
    {}
    
    virtual ~ThreadedDefLoader()
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

    // Ensures that the worker thread has been started and is done processing
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
            if (_result.valid())
            {
                _result.get();
            }

            if (_finisher.valid())
            {
                _finisher.get();
            }

            _result = std::shared_future<ReturnType>();
            _finisher = std::shared_future<void>();

            _loadingStarted = false;
        }
    }

    FinishedSignal& signal_finished()
    {
        return _finishedSignal;
    }

private:
    struct FinishSignalEmitter
    {
        FinishedSignal& _signal;
        std::shared_future<void>& _targetFuture;

        FinishSignalEmitter(FinishedSignal& signal, std::shared_future<void>& targetFuture) :
            _signal(signal),
            _targetFuture(targetFuture)
        {}

        ~FinishSignalEmitter()
        {
            _targetFuture = std::async(std::launch::async, std::bind(&FinishedSignal::emit, _signal));
        }
    };

    void ensureLoaderStarted()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_loadingStarted)
        {
            _loadingStarted = true;
            _result = std::async(std::launch::async, [&]()
            {
                // When going out of scope, this instance invokes the finished signal in a separate thread
                FinishSignalEmitter finisher(_finishedSignal, _finisher);
                return _loadFunc();
            });
        }
    }
};

}
