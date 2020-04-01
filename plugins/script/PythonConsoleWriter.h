#pragma once

#include "itextstream.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace script 
{

/**
 * greebo: A small helper class for redirecting Python's StdIo output
 * to DarkRadiant's global*() streams.
 *
 * The output is duplicated to a given string.
 */
class PythonConsoleWriter 
{
private:
	// whether to write to output or error stream
	bool _isErrorLogger;

	// A buffer keeping the messages until read
	std::string& _outputBuffer;

public:
	PythonConsoleWriter(bool isErrorLogger, std::string& outputBuffer) :
		_isErrorLogger(isErrorLogger),
		_outputBuffer(outputBuffer)
	{}

	// StdOut redirector
    void write(const std::string& msg)
	{
		_outputBuffer.append(msg);

		// Python doesn't send entire lines, it may send single characters,
		// so don't add std::endl each time
		if (_isErrorLogger)
		{
			rError() << msg;
		}
		else
		{
			rMessage() << msg;
		}
    }

	// Since this class serves as "stdout" it should implement flush()
	void flush()
	{
		// nothing to do here
	}
};
typedef py::class_<PythonConsoleWriter> PythonConsoleWriterClass;

} // namespace script
