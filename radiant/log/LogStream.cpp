#include "LogStream.h"

#include "itextstream.h"
#include "COutRedirector.h"
#include "StringLogDevice.h"

namespace applog
{

std::mutex LogStream::_streamLock;

LogStream::LogStream(ELogLevel logLevel) :
    std::ostream(new LogStreamBuf(logLevel))
{}

LogStream::~LogStream()
{
    LogStreamBuf* buf = static_cast<LogStreamBuf*>(rdbuf());

    if (buf != nullptr)
    {
        delete buf;
    }
}

std::ostream& getGlobalOutputStream() {
	static applog::LogStream _stream(SYS_STANDARD);
	return _stream;
}

std::ostream& getGlobalErrorStream() {
	static applog::LogStream _stream(SYS_ERROR);
	return _stream;
}

std::ostream& getGlobalWarningStream() {
	static applog::LogStream _stream(SYS_WARNING);
	return _stream;
}

void LogStream::InitialiseStreams()
{
    // Instantiate a temporary buffer, which copies the log until the
    // console is ready. The buffer's contents will then be copied over
    StringLogDevice::InstancePtr() = std::make_shared<StringLogDevice>();

	GlobalOutputStream().setStream(getGlobalOutputStream());
	GlobalWarningStream().setStream(getGlobalWarningStream());
	GlobalErrorStream().setStream(getGlobalErrorStream());

#ifndef NDEBUG
    GlobalDebugStream().setStream(getGlobalOutputStream());
#endif

    GlobalOutputStream().setLock(GetStreamLock());
    GlobalWarningStream().setLock(GetStreamLock());
    GlobalErrorStream().setLock(GetStreamLock());
    GlobalDebugStream().setLock(GetStreamLock());

#if !defined(POSIX) || !defined(_DEBUG)
	// Redirect std::cout to the log, except on Linux debug builds where
    // logging to the console is more useful
	COutRedirector::init();
#endif
}

void LogStream::ShutdownStreams()
{
#if !defined(POSIX) || !defined(_DEBUG)
	// Stop redirecting std::cout
	COutRedirector::destroy();
#endif
}

std::mutex& LogStream::GetStreamLock()
{
    return _streamLock;
}

} // namespace applog
