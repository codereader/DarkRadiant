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
class LogStream : 
	public std::ostream
{
public:
	LogStream(ELogLevel logLevel) : 
		std::ostream(new LogStreamBuf(logLevel))
	{}

	virtual ~LogStream() {
		LogStreamBuf* buf = static_cast<LogStreamBuf*>(rdbuf());
		if (buf != NULL) {
			delete buf;
		}
	}
};

// Accessors to the singleton log streams
std::ostream& getGlobalOutputStream();
std::ostream& getGlobalErrorStream();
std::ostream& getGlobalWarningStream();

// Gets called immediately after entering main()
// Sets up the stream references for globalOutputStream(), redirects std::cout, etc.
void initialiseLogStreams();

// Hands back the original streambuf to std::cout
void shutdownStreams();

} // namespace applog

#endif /* _LOG_STREAM_H_ */
