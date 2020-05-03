#include "LogStream.h"

#include "itextstream.h"
#include "COutRedirector.h"
#include "StringLogDevice.h"

namespace applog
{

std::mutex LogStream::_streamLock;

LogStream::LogStream(LogLevel logLevel) :
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

void LogStream::InitialiseStreams(ILogWriter& logWriter)
{
    // Instantiate a temporary buffer, which copies the log until the
    // console is ready. The buffer's contents will then be copied over
    StringLogDevice::InstancePtr() = std::make_shared<StringLogDevice>();

	GlobalOutputStream().setStream(logWriter.getLogStream(applog::LogLevel::Standard));
	GlobalWarningStream().setStream(logWriter.getLogStream(applog::LogLevel::Warning));
	GlobalErrorStream().setStream(logWriter.getLogStream(applog::LogLevel::Error));

#ifndef NDEBUG
    GlobalDebugStream().setStream(logWriter.getLogStream(applog::LogLevel::Verbose));
#endif

    GlobalOutputStream().setLock(logWriter.getStreamLock());
    GlobalWarningStream().setLock(logWriter.getStreamLock());
    GlobalErrorStream().setLock(logWriter.getStreamLock());
    GlobalDebugStream().setLock(logWriter.getStreamLock());

#if !defined(POSIX) || !defined(_DEBUG)
	// Redirect std::cout to the log, except on Linux debug builds where
    // logging to the console is more useful
	COutRedirector::init(logWriter);
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
