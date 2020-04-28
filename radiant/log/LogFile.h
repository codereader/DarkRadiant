#pragma once

#include <fstream>
#include "ilogwriter.h"

namespace applog
{

class LogFile :
	public ILogDevice
{
	// The log file name including path
	std::string _logFilePath;

    // We write line by line
    std::string _buffer;

	// The file stream which will be filled with bytes
	std::ofstream _logStream;

public:
	/**
	 * greebo: Pass the full path to the constructor
	 */
	LogFile(const std::string& fullPath);

	virtual ~LogFile();

	// Returns true if the log stream was successfully opened
	bool isOpen();

	// Returns the full path to the log file
	const std::string& getFullPath() const;

	/**
	 * Use this to write a string to the logfile. This usually gets
	 * called by the LogWriter class, but it can be called independently.
	 */
	void writeLog(const std::string& outputStr, ELogLevel level) override;
};

} // namespace applog
