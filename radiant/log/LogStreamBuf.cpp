#include "LogStreamBuf.h"

#include <stdexcept>
#include "LogWriter.h"

namespace applog {

LogStreamBuf::LogStreamBuf(int level, int bufferSize) :
	_reserve(NULL),
	_level(static_cast<ELogLevel>(level))
{
	// Sanity-check the given int
	if (level < 0 || level >= static_cast<int>(SYS_NUM_LOGLEVELS)) {
		throw std::logic_error("Cannot instantiate LogStreamBuf (invalid level).");
	}

	if (bufferSize) {
		_reserve = new char[bufferSize];
		setp(_reserve, _reserve + bufferSize);
	}
	else {
		setp(NULL, NULL);
	}

	// No input buffer, set this to NULL
	setg(NULL, NULL, NULL);
}

LogStreamBuf::~LogStreamBuf() {
	sync();

	if (_reserve != NULL) {
		delete[] _reserve;
	}
}

// These two get called by the base class streambuf
LogStreamBuf::int_type LogStreamBuf::overflow(int_type c) {
	// Write the buffer
	writeToBuffer();

	if (c != EOF) {
		if (pbase() == epptr()) {
			// Write just this single character
			int c1 = c;

			LogWriter::Instance().write(reinterpret_cast<const char*>(&c1), 1, _level);
		}
		else {
			sputc(c);
		}
	}

	return 0;
}

LogStreamBuf::int_type LogStreamBuf::sync() {
	writeToBuffer();
	return 0;
}

void LogStreamBuf::writeToBuffer() {
	int_type charsToWrite = pptr() - pbase();
	
	if (pbase() != pptr()) {
		// Write the given characters to the GtkTextBuffer
		LogWriter::Instance().write(_reserve, static_cast<std::size_t>(charsToWrite), _level);

		setp(pbase(), epptr());
	}
}

} // namespace applog
