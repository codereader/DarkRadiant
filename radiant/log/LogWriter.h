#pragma once

#include <set>
#include "LogLevels.h"
#include "LogDevice.h"

namespace applog {

class LogWriter
{
	// The set of unique log devices
	typedef std::set<LogDevice*> LogDevices;
	LogDevices _devices;

public:
	/**
	 * greebo: Writes the given buffer p with the given length to the
	 *         various output devices (i.e. Console and Log file).
	 */
	void write(const char* p, std::size_t length, ELogLevel level);

	/**
	 * greebo: Use these methods to attach/detach a log device from the
	 *         writer class. After attaching a device, all log output
	 *         will be written to it.
	 */
	void attach(LogDevice* device);
	void detach(LogDevice* device);

	// Contains the static singleton instance of this writer
	static LogWriter& Instance();
};

} // namespace applog
