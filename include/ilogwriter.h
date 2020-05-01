#pragma once

#include <string>
#include <mutex>

namespace applog
{

enum class LogLevel
{
	Verbose = 0,
	Standard,
	Warning,
	Error,
};

constexpr LogLevel AllLogLevels[] = {
	LogLevel::Verbose,
	LogLevel::Standard,
	LogLevel::Warning,
	LogLevel::Error
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
	virtual void writeLog(const std::string& outputStr, LogLevel level) = 0;

	/**
	 * If this device is a console, it will be filled with the log output
	 * that has been collected before it has been attached.
	 * This is supposed to make it easier to inspect the startup phase opening
	 * the console viewer in the UI.
	 */
	virtual bool isConsole() const 
	{
		return false;
	}
};

/**
 * Central logging hub, dispatching any incoming log messages 
 * to all attached ILogDevices.
 *
 * Also offers the stream references to set up the rMessage/rWarning streams
 * in every module.
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
	virtual void write(const char* p, std::size_t length, LogLevel level) = 0;

	/**
	 * Returns the stream reference for the given log level. These are used
	 * to initialise the global rMessage/rWarning streams used by the application.
	 */
	virtual std::ostream& getLogStream(LogLevel level) = 0;

	/**
	 * Returns the synchronization object for writing to the streams.
	 */
	virtual std::mutex& getStreamLock() = 0;

	/**
	 * greebo: Use these methods to attach/detach a log device from the
	 * writer class. After attaching a device, all log output
	 * will be written to it.
	 */
	virtual void attach(ILogDevice* device) = 0;
	virtual void detach(ILogDevice* device) = 0;
};

}
