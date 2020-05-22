#pragma once

#include <stdexcept>

namespace cmd
{

/**
 * Exception type thrown when an operation failed.
 * Any function executed by the CommandSystem can throw this type,
 * it will be caught by the CommandSystem and possibly end up
 * displayed in an error popup window.
 */
class ExecutionFailure :
	public std::runtime_error
{
public:
	ExecutionFailure(const std::string& msg) :
		std::runtime_error(msg)
	{}
};

}
