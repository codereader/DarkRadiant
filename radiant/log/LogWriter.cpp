#include "LogWriter.h"

#include <gtk/gtk.h>
#include "gtkutil/IConv.h"

#include "LogFile.h"
#include "Console.h"

namespace applog {

LogWriter::LogWriter() :
	_buffer(NULL)
{}

void LogWriter::write(const char* p, std::size_t length, ELogLevel level) {
	// Write to the logfile if it is available
	if (LogFile::InstancePtr() != NULL) {
		LogFile::InstancePtr()->write(p, length);
	}
	
	ui::Console::Instance().write(p, length, level);
}

void LogWriter::disconnectConsoleWindow() {
	_buffer = NULL;
}

LogWriter& LogWriter::Instance() {
	static LogWriter _writer;
	return _writer;
}

// Initialise the static members
GtkTextTag* LogWriter::errorTag = NULL;
GtkTextTag* LogWriter::warningTag = NULL;
GtkTextTag* LogWriter::standardTag = NULL;

} // namespace applog
