#pragma once

#include <mutex>
#include <list>
#include <functional>
#include <future>

namespace util
{

class TaskQueue
{
private:
    std::mutex _lock;

    std::list<std::function<void()>> _queue;

    std::future<void> _current;

public:
    ~TaskQueue()
    {
        clear();
    }

    // Adds the given task to the queue. This will launch the task
    // immediately if no other task is currently processed
    void enqueue(const std::function<void()>& task)
    {
        {
            std::lock_guard<std::mutex> lock(_lock);
            _queue.push_front(task);
        }

        if (isIdle())
        {
            startNextTask();
        }
    }

    // Clears the queue. This might block waiting for any currently 
    // active taks to finish
    void clear()
    {
        {
            // Lock the queue and remove any tasks such that no new ones are added
            std::lock_guard<std::mutex> lock(_lock);
            _queue.clear();
        }

        // Clear (and possibly) wait for the currently active future object
        _current = std::future<void>();
    }

private:
    bool isIdle() const
    {
        return !_current.valid() || 
            _current.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void startNextTask()
    {
        std::lock_guard<std::mutex> lock(_lock);

        if (_queue.empty())
        {
            return;
        }

        // No active task, dispatch a new one
        auto frontOfQueue = _queue.front();
        _queue.pop_front();

        // Wrap the given task in our own lambda to start the next task right afterwards
        _current = std::async(std::launch::async, [this, frontOfQueue]()
        {
            frontOfQueue();
            startNextTask();
        });
    }
};

}
