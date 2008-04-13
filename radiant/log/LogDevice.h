#ifndef _LOG_DEVICE_H_
#define _LOG_DEVICE_H_

#include <string>
#include "LogLevels.h"

namespace applog {

/** 
 * greebo: A LogDevice is a class which is able to take log output.
 *
 * Examples of LogDevices are the Console and the DarkRadiant logfile.
 * Note: Use the LogWriter::attach() method to register a class for logging.
 */
class LogDevice {
public:
	/**
	 * greebo: This method gets called by the Writer with 
	 * a logging string as argument.
	 */
	virtual void writeLog(const std::string& outputStr, ELogLevel level) = 0;
};

} // namespace applog

#endif /* _LOG_DEVICE_H_ */
