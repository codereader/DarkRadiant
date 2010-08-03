#include "LogFile.h"

#include <gtk/gtkmain.h>

#include "imodule.h"
#include "itextstream.h"
#include "version.h"

#include "string/string.h"
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
	if (_logStream.good()) {
		// Register this class as logdevice
		LogWriter::Instance().attach(this);
	}
	else {
		std::cerr << "Failed to create log file, check write permissions in Radiant directory." << std::endl;
	}
}

LogFile::~LogFile() {
	time_t localtime;
	time(&localtime);

	globalOutputStream() << "Closing log file at " << ctime(&localtime) << std::endl;
	
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
void LogFile::create(const std::string& filename) {
	if (InstancePtr() == NULL) {
		// No logfile yet, create one
		InstancePtr() = LogFilePtr(new LogFile(filename));

		// Write the initialisation info to the logfile.
		globalOutputStream() << "Started logging to " << InstancePtr()->_logFilename << std::endl;

		time_t localtime;
		time(&localtime);
		globalOutputStream() << "Today is: " << ctime(&localtime) 
			                 << "This is " << RADIANT_APPNAME_FULL() << std::endl;
        
		// Output the GTK+ version to the logfile
        std::string gtkVersion = intToStr(gtk_major_version) + "."; 
		gtkVersion += intToStr(gtk_minor_version) + "."; 
		gtkVersion += intToStr(gtk_micro_version);

        globalOutputStream() << "GTK+ Version: " << gtkVersion << std::endl;
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
