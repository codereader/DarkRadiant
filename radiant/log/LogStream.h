#pragma once

#include <ostream>
#include <mutex>
#include "LogStreamBuf.h"

namespace applog {

/**
 * greebo: A LogStream is a specialised std::ostream making
 * use of a customised std::streambuf (LogStreamBuf). 
 *
 * Several application-wide LogStream instances exist, one for
 * each LogLevel. The underlying LogStreamBuf will write
 * all output to the LogWriter singleton, which in turn
 * dispatches the text to the various LogDevices.
 *
 * Application code needs to include the itextstream.h header
 * and use the rMessage(), rWarning() and rError() shortcuts
 * to stream data to the log.
 */
class LogStream :
	public std::ostream
{
private:
    static std::mutex _streamLock;
public:
    LogStream(ELogLevel logLevel);

    virtual ~LogStream();

    // Gets called immediately after entering main()
    // Sets up the stream references for rMessage(), redirects std::cout, etc.
    static void InitialiseStreams();

    // Hands back the original streambuf to std::cout
    static void ShutdownStreams();

    // The one and only lock for logging
    static std::mutex& GetStreamLock();
};

// Accessors to the singleton log streams
std::ostream& getGlobalOutputStream();
std::ostream& getGlobalErrorStream();
std::ostream& getGlobalWarningStream();

} // namespace applog
