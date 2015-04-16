#include "LogFile.h"

#include <iomanip>
#include <thread>
#include <wx/version.h>
#include "imodule.h"
#include "itextstream.h"
#include "version.h"

#include "string/convert.h"
#include "LogWriter.h"
#include "modulesystem/ModuleRegistry.h"

namespace applog
{

namespace
{
    const char* const TIME_FMT = "%Y-%m-%d %H:%M:%S";
}

LogFile::LogFile(const std::string& filename) :
	_logFilename(
		module::ModuleRegistry::Instance().getApplicationContext().getSettingsPath() +
		filename
	),
    _logStream(_logFilename.c_str())
{
	if (_logStream.good())
	{
		// Register this class as logdevice
		LogWriter::Instance().attach(this);
	}
	else
	{
        rConsoleError() << "Failed to create log file '"
				  << _logFilename << ", check write permissions in parent directory." 
				  << std::endl;
    }
}

LogFile::~LogFile()
{
#if defined(__linux__)
	// put_time still unavailable even with GCC 4.9
    rMessage() << " Closing log file." << std::endl;
#else
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    rMessage() << std::put_time(&tm, TIME_FMT) << " Closing log file." << std::endl;
#endif

    // Insert the last few remaining bytes into the stream
    if (!_buffer.empty())
    {
        _logStream << _buffer << std::endl;
        _buffer.clear();
    }

	_logStream.flush();
	_logStream.close();

	LogWriter::Instance().detach(this);
}

void LogFile::writeLog(const std::string& outputStr, ELogLevel level) 
{
    _buffer.append(outputStr);

    // Hold back until we hit a newline
    if (outputStr.rfind('\n') != std::string::npos)
    {
#if !defined(__linux__)
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);

        // Write timestamp and thread information
        _logStream << std::put_time(&tm, TIME_FMT);
#endif

        _logStream << " (" << std::this_thread::get_id() << ") ";

        // Insert the string into the stream and flush the buffer
        _logStream << _buffer;

        _buffer.clear();
        _logStream.flush();
    }
}

// Creates the singleton logfile with the given filename
void LogFile::create(const std::string& filename)
{
	if (InstancePtr() == NULL)
    {
		// No logfile yet, create one
		InstancePtr() = LogFilePtr(new LogFile(filename));

		// Write the initialisation info to the logfile.
		rMessage() << "Started logging to " << InstancePtr()->_logFilename << std::endl;

		rMessage() << "This is " << RADIANT_APPNAME_FULL() << std::endl;

#if !defined(__linux__)
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);

        // Write timestamp and thread information
        rMessage() << "Today is " << std::put_time(&tm, TIME_FMT) << std::endl;
#endif

		// Output the wxWidgets version to the logfile
        std::string wxVersion = string::to_string(wxMAJOR_VERSION) + ".";
		wxVersion += string::to_string(wxMINOR_VERSION) + ".";
		wxVersion += string::to_string(wxRELEASE_NUMBER);

        rMessage() << "wxWidgets Version: " << wxVersion << std::endl;
	}
}

// Closes the singleton log instance
void LogFile::close() {
	// Clear the pointer, this destructs any open logfile instance
	InstancePtr() = LogFilePtr();
}

LogFilePtr& LogFile::InstancePtr() {
	static LogFilePtr _instancePtr;
	return _instancePtr;
}

} // namespace applog
