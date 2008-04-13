#include "LogStream.h"

#include "itextstream.h"
#include "COutRedirector.h"

namespace applog {

std::ostream& getGlobalOutputStream() {
	static applog::LogOutputStream _stream;
	return _stream;
}

std::ostream& getGlobalErrorStream() {
	static applog::LogErrorStream _stream;
	return _stream;
}

std::ostream& getGlobalWarningStream() {
	static applog::LogWarningStream _stream;
	return _stream;
}

void initialiseLogStreams() {
	GlobalErrorStream::instance().setOutputStream(getGlobalErrorStream());
	GlobalOutputStream::instance().setOutputStream(getGlobalOutputStream());

	// Redirect std::cout to the log
	COutRedirector::init();
}

void shutdownStreams() {
	// Stop redirecting std::cout
	COutRedirector::destroy();
}

} // namespace applog
