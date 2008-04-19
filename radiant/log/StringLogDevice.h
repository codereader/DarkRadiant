#ifndef _STRING_LOG_DEVICE_H_
#define _STRING_LOG_DEVICE_H_

#include <sstream>
#include "LogDevice.h"
#include <boost/shared_ptr.hpp>

namespace applog {

class StringLogDevice;
typedef boost::shared_ptr<StringLogDevice> StringLogDevicePtr;

/** 
 * greebo: A StringLogDevice is a class which logs into a local string buffer.
 */
class StringLogDevice :
	public LogDevice
{
	std::ostringstream _errorStream;
	std::ostringstream _warningStream;
	std::ostringstream _logStream;
public:
	StringLogDevice();
	~StringLogDevice();

	/**
	 * greebo: This method gets called by the Writer with 
	 * a logging string as argument.
	 */
	void writeLog(const std::string& outputStr, ELogLevel level);

	// Returns the temporary buffer for the given level
	std::string getString(ELogLevel level);

	// Destroys the static instance
	static void destroy();

	static StringLogDevicePtr& InstancePtr();
};

} // namespace applog

#endif /* _STRING_LOG_DEVICE_H_ */
