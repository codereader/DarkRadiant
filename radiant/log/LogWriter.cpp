#include "LogWriter.h"

namespace applog {

void LogWriter::write(const char* p, std::size_t length, ELogLevel level) {
	// Convert the buffer to a string
	std::string output(p, length);

	// Visit all the logfiles and write the string
	for (LogDevices::iterator i = _devices.begin(); i != _devices.end(); i++) {
		(*i)->writeLog(output, level);
	}
}

void LogWriter::attach(LogDevice* device) {
	_devices.insert(device);
}

void LogWriter::detach(LogDevice* device) {
	_devices.erase(device);
}

LogWriter& LogWriter::Instance() {
	static LogWriter _writer;
	return _writer;
}

} // namespace applog
