#ifndef _LOG_STREAM_H_
#define _LOG_STREAM_H_

#include <ostream>
#include "LogStreamBuf.h"

namespace applog {

/**
 * greebo: A LogStream can be used to write stuff to the console, 
 *         the logfile and every other logging "device" attached to 
 *         the LogWriter class.
 *
 * The LogStream uses a LogStreamBuf as buffer class, which itself
 * invokes the LogWriter class.
 */
template<int LogLevel>
class LogStream : 
	public std::ostream
{
public:
	LogStream() : 
		std::ostream(new LogStreamBuf(LogLevel))
	{}

	virtual ~LogStream() {
		LogStreamBuf* buf = static_cast<LogStreamBuf*>(rdbuf());
		if (buf != NULL) {
			delete buf;
		}
	}
};

// Shorthand typedefs for the various LogLevels
typedef LogStream<SYS_STANDARD> LogOutputStream;
typedef LogStream<SYS_ERROR>    LogErrorStream;
typedef LogStream<SYS_WARNING>  LogWarningStream;

} // namespace applog

#endif /* _LOG_STREAM_H_ */
