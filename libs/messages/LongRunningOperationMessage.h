#pragma once

#include "imessagebus.h"

namespace radiant
{

enum class OperationEvent
{
	Started,
	Finished,
};

/**
 * Message object sent to the MessageBus when one of the
 * modules are performing a long-running operation.
 *
 * UI modules can react to this by showing a blocking
 * window, depending on the event type as returned by getType().
 *
 * See also ScopedLongRunningOperation.h
 */
class LongRunningOperationMessage :
	public radiant::IMessage
{
private:
	OperationEvent _event;

	std::string _message;

public:
	LongRunningOperationMessage(OperationEvent ev) :
		LongRunningOperationMessage(ev, std::string())
	{}

	LongRunningOperationMessage(OperationEvent ev, const std::string& message) :
		_event(ev),
		_message(message)
	{}

	OperationEvent getType() const
	{
		return _event;
	}

	const std::string& getMessage() const
	{
		return _message;
	}
};

}
