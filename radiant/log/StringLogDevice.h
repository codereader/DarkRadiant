#pragma once

#include <sstream>
#include <memory>
#include "ilogwriter.h"

namespace applog
{

/**
 * greebo: A StringLogDevice is a class which logs into a local string buffer.
 */
class StringLogDevice :
	public ILogDevice
{
private:
	std::ostringstream _errorStream;
	std::ostringstream _warningStream;
	std::ostringstream _logStream;

public:
	typedef std::shared_ptr<StringLogDevice> Ptr;

	StringLogDevice();
	~StringLogDevice();

	/**
	 * greebo: This method gets called by the Writer with
	 * a logging string as argument.
	 */
	void writeLog(const std::string& outputStr, ELogLevel level) override;

	// Returns the temporary buffer for the given level
	std::string getString(ELogLevel level);

	// Destroys the static instance
	static void destroy();

	static Ptr& InstancePtr();
};

} // namespace applog
