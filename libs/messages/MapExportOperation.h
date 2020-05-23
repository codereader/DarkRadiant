#pragma once

#include <stdexcept>
#include "imessagebus.h"

namespace map
{

/**
 * Message sent when the export code is about to start writing
 * to its output stream.
 * 
 * If any listener wishes to cancel the operation, the cancel() 
 * method is available, which will throw an internal exception.
 */
class ExportOperation :
    public radiant::IMessage
{
public:
    enum EventType
    {
        Started,  // operation started, called once
        Progress, // operation in progress, called 0..N times
        Finished, // operation finished, called once
    };

    class OperationCancelled :
        public std::runtime_error
    {
    public:
        OperationCancelled() :
            runtime_error("")
        {}
    };

private:
    EventType _type;
    float _progressFraction;
    std::size_t _numTotalNodes;
    std::string _message;

public:
    ExportOperation(EventType type, std::size_t numTotalNodes) :
        ExportOperation(type, numTotalNodes, 0)
    {}

    ExportOperation(EventType type, std::size_t numTotalNodes, float progressFraction) :
        _type(type),
        _progressFraction(progressFraction),
        _numTotalNodes(numTotalNodes)
    {}

    const std::string& getText() const
    {
        return _message;
    }

    void setText(const std::string& message)
    {
        _message = message;
    }

    EventType getType() const
    {
        return _type;
    }

    float getProgressFraction() const
    {
        return _progressFraction;
    }

    std::size_t getNumTotalNodes() const
    {
        return _numTotalNodes;
    }

    // Call this to cancel the ongoing export process
    // This will throw an exception, so control won't be returned to the caller
    void cancelOperation()
    {
        throw OperationCancelled();
    }
};

}
