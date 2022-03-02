#pragma once

#include <future>
#include <functional>
#include <algorithm>
#include <vector>
#include "ifilesystem.h"

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
 * 
 * The loader will invoke the functor passed to the constructor for each encountered 
 * def file, in the same order as the idTech4/TDM engine (alphabetically).
 */
template <typename ReturnType>
class ThreadedDefLoader
{
private:
    std::string _baseDir;
    std::string _extension;
    std::size_t _depth;

    typedef std::function<ReturnType()> LoadFunction;

    LoadFunction _loadFunc;
    std::function<void()> _finishedFunc;

    std::shared_future<ReturnType> _result;
    std::shared_future<void> _finisher;
    std::mutex _mutex;

    bool _loadingStarted;

public:
    ThreadedDefLoader(const std::string& baseDir, const std::string& extension, const LoadFunction& loadFunc) :
        ThreadedDefLoader(baseDir, extension, 0, loadFunc)
    {}
    
    ThreadedDefLoader(const std::string& baseDir, const std::string& extension, std::size_t depth, const LoadFunction& loadFunc) :
        ThreadedDefLoader(baseDir, extension, depth, loadFunc, std::function<void()>())
    {}

    ThreadedDefLoader(const std::string& baseDir, const std::string& extension, std::size_t depth,
                      const LoadFunction& loadFunc, const std::function<void()>& finishedFunc) :
        _baseDir(baseDir),
        _extension(extension),
        _depth(depth),
        _loadFunc(loadFunc),
        _finishedFunc(finishedFunc),
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
            _loadingStarted = false;

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
        }
    }

protected:
    void loadFiles(const vfs::VirtualFileSystem::VisitorFunc& visitor)
    {
        loadFiles(GlobalFileSystem(), visitor);
    }

    void loadFiles(vfs::VirtualFileSystem& vfs, const vfs::VirtualFileSystem::VisitorFunc& visitor)
    {
        // Accumulate all the files and sort them before calling the visitor
        std::vector<vfs::FileInfo> _incomingFiles;
        _incomingFiles.reserve(200);

        vfs.forEachFile(_baseDir, _extension, [&](const vfs::FileInfo& info)
        {
            _incomingFiles.push_back(info);
        }, _depth);

        // Sort the files by name
        std::sort(_incomingFiles.begin(), _incomingFiles.end(), [](const vfs::FileInfo& a, const vfs::FileInfo& b)
        {
            return a.name < b.name;
        });

        // Dispatch the sorted list to the visitor
        std::for_each(_incomingFiles.begin(), _incomingFiles.end(), visitor);
    }

private:
    struct FinishFunctionCaller
    {
        std::function<void()> _function;
        std::shared_future<void>& _targetFuture;

        FinishFunctionCaller(const std::function<void()>& function, std::shared_future<void>& targetFuture) :
            _function(function),
            _targetFuture(targetFuture)
        {}

        ~FinishFunctionCaller()
        {
            if (_function)
            {
                _targetFuture = std::async(std::launch::async, _function);
            }
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
                // When going out of scope, this instance invokes the finished callback in a separate thread
                FinishFunctionCaller finisher(_finishedFunc, _finisher);
                return _loadFunc();
            });
        }
    }
};

}
