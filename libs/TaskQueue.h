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
    mutable std::mutex _queueLock;
    std::list<std::function<void()>> _queue;

    mutable std::mutex _currentLock;
    std::future<void> _current;
    std::future<void> _finished;

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
            std::lock_guard<std::mutex> lock(_queueLock);
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
            std::lock_guard<std::mutex> lock(_queueLock);
            _queue.clear();
        }

        _current = std::future<void>();
        _finished = std::future<void>();
    }

private:
    bool isIdle() const
    {
        std::lock_guard<std::mutex> lock(_currentLock);
        return !_current.valid() || _current.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    std::function<void()> dequeueOne()
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        if (_queue.empty())
        {
            return std::function<void()>();
        }

        // No active task, dispatch a new one
        auto frontOfQueue = _queue.front();
        _queue.pop_front();

        return frontOfQueue;
    }

    void startNextTask()
    {
        auto task = dequeueOne();

        if (!task)
        {
            return;
        }

        // Wrap the given task in our own lambda to start the next task right afterwards
        std::lock_guard<std::mutex> lock(_currentLock);
        _current = std::async(std::launch::async, [this, task]()
        {
            task();

            {
                // Move our own task to the finished lane, 
                // to avoid blocking when assigning a new future
                std::lock_guard<std::mutex> lock(_currentLock);
                _finished = std::move(_current);
            }

            // _current is now empty, so we can start a new task
            startNextTask();
        });
    }
};

}
