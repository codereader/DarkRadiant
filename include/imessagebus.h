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

    // Returns a unique ID of this message, see IMessage::Type enum
    // for well-known types
    virtual std::size_t getId() const = 0;

    // Returns true if this message has been handled
    bool isHandled() const
    {
        return _handled;
    }

    void setHandled(bool handled)
    {
        _handled = handled;
    }

public:
    // Pre-defined message type IDs.
    // Plugin code can define their own IDs in the range of 1000+
    enum Type : std::size_t
    {
        ApplicationShutdownRequest,
        AutomaticMapSaveRequest,
        ClearConsole,
        CommandExecutionFailed,
        GameConfigNeeded,
        LongRunningOperation,
        MapFileOperation,
        MapOperationFinished,
        FileSelectionRequest,
        Notification,
        TextureChanged,
        ApplicationIsActiveQuery,
        FileOverwriteConfirmation,
        FileSaveConfirmation,
        UnselectSelectionRequest,
        ManipulatorModeToggleRequest,
        ComponentSelectionModeToggleRequest,
        GridSnapRequest,
        FocusMaterialRequest,
        TextureToolRequest,

        UserDefinedMessagesGoHigherThanThis = 999,
    };
};

/**
 * General-purpose handler used to send and process message to registered listeners.
 * Client code can send arbitrary message objects as long as they are deriving from 
 * IMessage. 
 * Listeners need to subscribe to each message type they're interested in.
 * To receive the message, client code needs to add a listener, which can either 
 * listen for a specific type of messages or all of them.
 */
class IMessageBus
{
public:
    virtual ~IMessageBus() {}

    typedef std::function<void(IMessage&)> Listener;

    /**
     * Registers a listener that is only called when the give message type
     * is sent across the wire. Use the returned ID to unsubscribe the listener.
     */
    virtual std::size_t addListener(std::size_t messageType, const Listener& listener) = 0;

    /**
     * Unsubscribe the given listener.
     */
    virtual void removeListener(std::size_t listenerId) = 0;

    /**
     * Send the given message to the given channel. The channel ID refers to 
     * a given message type and must have been acquired using getChannelId() beforehand.
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
    {
        static_assert(std::is_base_of<IMessage, T>::value, "Listener must accept IMessage types");
    }

    TypeListener(const std::function<void(T&)>& specialisedFunc) :
        std::function<void(T&)>(specialisedFunc)
    {
        static_assert(std::is_base_of<IMessage, T>::value, "Listener must accept IMessage types");
    }

    // Fulfills the Listener function signature
    void operator()(IMessage& message)
    {
        // Perform a type cast and dispatch to our base class' call operator
        std::function<void(T&)>::operator()(static_cast<T&>(message));
    }
};

}
