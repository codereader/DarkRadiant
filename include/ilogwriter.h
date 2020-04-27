#pragma once

#include <string>

namespace applog
{

enum ELogLevel
{
	SYS_VERBOSE = 0,
	SYS_STANDARD,
	SYS_WARNING,
	SYS_ERROR,
	SYS_NUM_LOGLEVELS,
};

/**
 * greebo: A LogDevice is a class which is able to take log output.
 *
 * Examples of LogDevices are the Console and the DarkRadiant logfile.
 * Note: Use the LogWriter::attach() method to register a class for logging.
 */
class ILogDevice
{
public:
	virtual ~ILogDevice() {}

	/**
	 * greebo: This method gets called by the ILogWriter with
	 * a log string as argument.
	 */
	virtual void writeLog(const std::string& outputStr, ELogLevel level) = 0;
};

/**
 * Central logging hub, dispatching any incoming log messages 
 * to all attached ILogDevices.
 */
class ILogWriter
{
public:
	virtual ~ILogWriter()
	{}

	/**
	 * greebo: Writes the given buffer p with the given length to the
	 * various output devices (i.e. Console and Log file).
	 */
	virtual void write(const char* p, std::size_t length, ELogLevel level) = 0;

	/**
	 * greebo: Use these methods to attach/detach a log device from the
	 * writer class. After attaching a device, all log output
	 * will be written to it.
	 */
	virtual void attach(ILogDevice* device) = 0;
	virtual void detach(ILogDevice* device) = 0;
};

}
