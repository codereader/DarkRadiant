#pragma once

#include <mutex>
#include "imessagebus.h"
#include "itextstream.h"

namespace radiant
{

class MessageBus :
	public IMessageBus
{
private:
    std::recursive_mutex _lock;

    // Listener and its registration handle
    typedef std::pair<std::size_t, Listener> ListenerPlusId;

    // Maps message types to Listeners
    std::multimap<std::size_t, ListenerPlusId> _listeners;

    bool _processingMessage;
    std::size_t _nextId;

public:
    MessageBus() : 
        _nextId(1)
    {}

    std::size_t addListener(std::size_t messageType, const Listener& listener) override
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);

        auto id = _nextId++;
        _listeners.emplace(messageType, std::make_pair(id, listener));

        return id;
    }

    void removeListener(std::size_t listenerId) override
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);

        for (auto it = _listeners.begin(); it != _listeners.end(); ++it)
        {
            if (it->second.first == listenerId)
            {
                _listeners.erase(it);
                return;
            }
        }

        rWarning() << "MessageBus: Could not locate listener with ID " << listenerId << std::endl;
    }

    void sendMessage(IMessage& message) override
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);

        // Get the message type ID
        auto msgId = message.getId();

        for (auto it = _listeners.lower_bound(msgId); 
             it != _listeners.end() && it != _listeners.upper_bound(msgId); /* in-loop */)
        {
            (*it++).second.second(message);
        }
    }
};

}
