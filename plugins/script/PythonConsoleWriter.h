#ifndef _PYTHON_CONSOLE_WRITER_H_
#define _PYTHON_CONSOLE_WRITER_H_

#include "itextstream.h"
#include <boost/python.hpp>

namespace script {

/**
 * greebo: A small helper class for redirecting Python's StdIo output
 * to DarkRadiant's error stream.
 */
class PythonConsoleWriter {

	// whether to write to output or error stream
	bool _isErrorLogger;

public:
	PythonConsoleWriter(bool isErrorLogger) :
		_isErrorLogger(isErrorLogger)
	{}

	// StdOut redirector
    void write(const std::string& msg) {
		// Python doesn't send entire lines, it may send single characters, 
		// so don't add std::endl each time
		if (_isErrorLogger) {
			globalErrorStream() << msg;
		}
		else {
			globalOutputStream() << msg;
		}
    }
};
typedef boost::python::class_<PythonConsoleWriter> PythonConsoleWriterClass;

} // namespace script

#endif /* _PYTHON_CONSOLE_WRITER_H_ */
