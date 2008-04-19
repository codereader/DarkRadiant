#include "LogStream.h"

#include "itextstream.h"
#include "COutRedirector.h"
#include "StringLogDevice.h"

namespace applog {

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

void initialiseLogStreams() {
	GlobalErrorStream::instance().setOutputStream(getGlobalErrorStream());
	GlobalOutputStream::instance().setOutputStream(getGlobalOutputStream());

	// Redirect std::cout to the log
	COutRedirector::init();

	// Instantiate a temporary buffer, which copies the log until the
	// GTK-based console is ready. The buffer's contents will then be copied over
	StringLogDevice::InstancePtr() = StringLogDevicePtr(new StringLogDevice);
}

void shutdownStreams() {
	// Stop redirecting std::cout
	COutRedirector::destroy();
}

} // namespace applog
