#pragma once

#include <stdexcept>

namespace selection
{

namespace algorithm
{

/**
 * Exception thrown by some algorithm routines if the desired
 * command cannot be executed due to the current state of the
 * scenegraph, selection or other reasons.
 */
class CommandNotAvailableException :
	public std::exception
{
private:
	std::string _what;
public:
	CommandNotAvailableException(const std::string& msg) :
		std::exception(),
		_what(msg)
	{}

	virtual const char* what() const throw() override
	{
		return _what.c_str();
	}

	// Converts a function that is throwing a CommandNotAvailableException
	// to one that returns false instead (true if no exception is thrown)
	static bool ToBool(const std::function<void()>& func)
	{
		try
		{
			func();
			return true;
		}
		catch (CommandNotAvailableException&)
		{
			return false;
		}
	}
};

}

}
