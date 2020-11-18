#pragma once

#include <map>
#include "imessagebus.h"
#include "itextstream.h"

namespace radiant
{

class MessageBus :
	public IMessageBus
{
private:
    // Listener and its registration handle
    using Channel = std::map<std::size_t, Listener>;
    
    // Maps message types to a set of listeners
    std::map<std::size_t, Channel> _channels;

    bool _processingMessage;
    std::size_t _nextId;

public:
    MessageBus() : 
        _nextId(1)
    {}

    std::size_t addListener(std::size_t messageType, const Listener& listener) override
    {
        auto channel = _channels.find(messageType);

        if (channel == _channels.end())
        {
            channel = _channels.emplace(messageType, Channel()).first;
        }

        auto subscriberId = _nextId++;
        channel->second.emplace(subscriberId, listener);

        return subscriberId;
    }

    void removeListener(std::size_t listenerId) override
    {
        for (auto& channel : _channels)
        {
            auto l = channel.second.find(listenerId);

            if (l != channel.second.end())
            {
                channel.second.erase(l);
                return;
            }
        }

        rWarning() << "MessageBus: Could not locate listener with ID " << listenerId << std::endl;
    }

    void sendMessage(IMessage& message) override
    {
        auto channel = _channels.find(message.getId());

        if (channel == _channels.end())
        {
            rWarning() << "MessageBus: No channel for message ID " << message.getId() << std::endl;
            return;
        }

        for (auto it = channel->second.begin();
             it != channel->second.end(); /* in-loop */)
        {
            (*it++).second(message);
        }
    }
};

}
