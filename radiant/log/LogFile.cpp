#include "LogFile.h"

#include <gtk/gtkmain.h>

#include "imodule.h"
#include "iradiant.h"
#include "iregistry.h"
#include "itextstream.h"
#include "version.h"

#include "string/string.h"
#include "gtkutil/messagebox.h"

namespace applog {

LogFile::LogFile(const std::string& filename) :
	_logFilename(GlobalRegistry().get(RKEY_SETTINGS_PATH) + "darkradiant.log"),
	_logStream(_logFilename.c_str())
{
	if (_logStream.good()) {
		globalOutputStream() << "Started logging to " << _logFilename << "\n";

		time_t localtime;
		time(&localtime);
		globalOutputStream() << "Today is: " << ctime(&localtime) 
			                 << "This is DarkRadiant " RADIANT_VERSION "\n";
        
		// Output the GTK+ version to the logfile
        std::string gtkVersion = intToStr(gtk_major_version) + "."; 
		gtkVersion += intToStr(gtk_minor_version) + "."; 
		gtkVersion += intToStr(gtk_micro_version);

        globalOutputStream() << "GTK+ Version: " << gtkVersion.c_str() << "\n";
	}
	else {
		gtk_MessageBox(0, "Failed to create log file, check write permissions in Radiant directory.\n",
			"Console logging", eMB_OK, eMB_ICONERROR );
	}
}

LogFile::~LogFile() {
	time_t localtime;
	time(&localtime);

	globalOutputStream() << "Closing log file at " << ctime(&localtime) << "\n";
	
	_logStream.flush();
	_logStream.close();
}

void LogFile::write(const std::string& str) {
	_logStream << str;
	_logStream.flush();
}

// Creates the singleton logfile with the given filename
void LogFile::create(const std::string& filename) {
	if (InstancePtr() != NULL) {
		// No logfile yet, create one
		InstancePtr() = LogFilePtr(new LogFile(filename));
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
