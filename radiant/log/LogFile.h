#ifndef _LOG_FILE_H_
#define _LOG_FILE_H_

#include <fstream>
#include <boost/shared_ptr.hpp>
#include "LogDevice.h"

namespace applog {

// Shared_ptr forward declaration
class LogFile;
typedef boost::shared_ptr<LogFile> LogFilePtr;

class LogFile :
	public LogDevice
{
	// The log file name including path
	std::string _logFilename;

	// The file stream which will be filled with bytes
	std::ofstream _logStream;

public:
	/**
	 * greebo: Pass only the file name to the constructor,
	 * the path is automatically loaded from the XMLRegistry.
	 * (The path in the registry key SETTINGS_PATH is used.)
	 */
	LogFile(const std::string& filename);

	~LogFile();

	/** 
	 * Use this to write a string to the logfile. This usually gets
	 * called by the LogWriter class, but it can be called independently.
	 */
	void writeLog(const std::string& outputStr, ELogLevel level);

	// Creates the singleton logfile with the given filename
	static void create(const std::string& filename);

	// Closes the singleton log instance
	static void close();

	// The singleton InstancePtr(), is NULL when the logfile is closed
	static LogFilePtr& InstancePtr();
};

} // namespace applog

#endif /* _LOG_FILE_H_ */
