#pragma once

#include <string>
#include <functional>

namespace radiant
{

class IMessage
{
private:
    bool _handled;

protected:
    IMessage() :
        _handled(false)
    {}

public:
    virtual ~IMessage() {}

    // Returns true if this message has been handled
    bool isHandled() const
    {
        return _handled;
    }

    void setHandled(bool handled)
    {
        _handled = handled;
    }
};

/**
 * General-purpose handler used to send and process message to registered listeners.
 * Client code can send arbitrary message objects as long as they are deriving from 
 * IMessage. 
 * To receive the message, client code needs to add a listener.
 */
class IMessageBus
{
public:
    virtual ~IMessageBus() {}

    typedef std::function<void(IMessage&)> Listener;

    /**
     * Register a listener which gets called with the message as argument.
     * An ID of the listener will be returned which can be used for unsubscription.
     */
    virtual std::size_t addListener(const Listener& listener) = 0;

    /**
     * Unsubscribe the given listener.
     */
    virtual void removeListener(std::size_t listenerId) = 0;

    /**
     * Send the given message along the wire. Clients need to construct
     * the message themselves, and check the possible result.
     */
    virtual void sendMessage(IMessage& message) = 0;
};

/**
 * Small adaptor to allow for less client setup code
 * when only listening for a certain IMessage subtype
 */
template<typename T>
class TypeListener :
    private std::function<void(T&)>
{
public:
    TypeListener(void (*specialisedFunc)(T&)) :
        std::function<void(T&)>(specialisedFunc)
    {}

    TypeListener(const std::function<void(T&)>& specialisedFunc) :
        std::function<void(T&)>(specialisedFunc)
    {}

    // Fulfills the Listener function signature
    void operator()(IMessage& message)
    {
        // Perform a type cast and dispatch to our base class' call operator
        try
        {
            std::function<void(T&)>::operator()(dynamic_cast<T&>(message));
        }
        catch (std::bad_cast&)
        {}
    }
};

}
