#include "LogWriter.h"

#include "LogFile.h"
#include "Console.h"

namespace applog {

void LogWriter::write(const char* p, std::size_t length, ELogLevel level) {
	// Convert the buffer to a string
	std::string output(p, length);

	// Write to the logfile if it is available
	if (LogFile::InstancePtr() != NULL) {
		LogFile::InstancePtr()->write(output);
	}
	
	// Write to the console
	ui::Console::Instance().write(output, level);
}

LogWriter& LogWriter::Instance() {
	static LogWriter _writer;
	return _writer;
}

} // namespace applog
