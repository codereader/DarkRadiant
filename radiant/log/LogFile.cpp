#include "LogFile.h"

#include <wx/version.h>
#include "imodule.h"
#include "itextstream.h"
#include "version.h"

#include "string/convert.h"
#include "LogWriter.h"
#include "modulesystem/ModuleRegistry.h"

namespace applog {

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
	time_t localtime;
	time(&localtime);

	rMessage() << "Closing log file at " << ctime(&localtime) << std::endl;

	_logStream.flush();
	_logStream.close();

	LogWriter::Instance().detach(this);
}

void LogFile::writeLog(const std::string& outputStr, ELogLevel level) {
	// Insert the string into the stream and flush the buffer
	_logStream << outputStr;
	_logStream.flush();
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

		time_t localtime;
		time(&localtime);
		rMessage() << "Today is: " << ctime(&localtime)
			                 << "This is " << RADIANT_APPNAME_FULL() << std::endl;

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
