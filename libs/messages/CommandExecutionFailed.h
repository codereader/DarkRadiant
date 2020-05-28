#pragma once

#include "imessagebus.h"
#include "command/ExecutionFailure.h"

namespace radiant
{

/**
 * Message object sent through the bus when an internal command
 * failed to execute. A reference to the thrown exception object 
 * is stored internally and shipped with it.
 */
class CommandExecutionFailedMessage :
	public IMessage
{
private:
	const cmd::ExecutionFailure& _exception;

public:
	CommandExecutionFailedMessage(const cmd::ExecutionFailure& exception) :
		_exception(exception)
	{}

	std::string getMessage() const
	{
		return _exception.what();
	}

	const cmd::ExecutionFailure& getException() const
	{
		return _exception;
	}
};

}
