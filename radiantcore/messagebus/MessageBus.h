#pragma once

#include <mutex>
#include "imessagebus.h"

namespace radiant
{

class MessageBus :
	public IMessageBus
{
private:
    std::recursive_mutex _lock;
    std::map<std::size_t, Listener> _listeners;

    bool _processingMessage;
    std::size_t _nextId;

public:
    MessageBus() : 
        _nextId(1)
    {}

    std::size_t addListener(const Listener & listener) override
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);

        auto id = _nextId++;
        _listeners.emplace(id, listener);

        return id;
    }

    void removeListener(std::size_t listenerId) override
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);

        assert(_listeners.find(listenerId) != _listeners.end());

        _listeners.erase(listenerId);
    }

    void sendMessage(IMessage& message) override
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);

        for (auto it = _listeners.begin(); it != _listeners.end(); /* in-loop */)
        {
            (*it++).second(message);
        }
    }
};

}
