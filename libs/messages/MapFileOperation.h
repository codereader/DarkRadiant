#pragma once

#include <stdexcept>
#include "imessagebus.h"

namespace map
{

/**
 * Message sent when the import/export code is about to start 
 * working on a stream.
 * 
 * If any listener wishes to cancel the operation, the cancel() 
 * method is available, which will throw an internal exception.
 */
class FileOperation :
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
    bool _canCalculateProgress;
    std::string _message;

public:
    FileOperation(EventType type, bool canCalculateProgress) :
        FileOperation(type, canCalculateProgress, 0.0f)
    {}

    FileOperation(EventType type, bool canCalculateProgress, float progressFraction) :
        _type(type),
        _progressFraction(progressFraction),
        _canCalculateProgress(canCalculateProgress)
    {
        if (_progressFraction < 0)
        {
            _progressFraction = 0;
        }
        else if (_progressFraction > 1.0f)
        {
            _progressFraction = 1.0f;
        }

        if (_type == Started)
        {
            _progressFraction = 0;
        }
        else if (_type == Finished)
        {
            _progressFraction = 1;
        }
    }

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

    // If canCalculateProgress() is true, this indicates the progress in the range of [0..1]
    float getProgressFraction() const
    {
        return _progressFraction;
    }

    // Whether the information in this message can be used to show a meaningful progress meter
    bool canCalculateProgress() const
    {
        return _canCalculateProgress;
    }

    // Call this to cancel the ongoing export process
    // This will throw an exception, so control won't be returned to the caller
    void cancelOperation()
    {
        throw OperationCancelled();
    }
};

}
