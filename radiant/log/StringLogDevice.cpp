#include "StringLogDevice.h"
#include "LogWriter.h"

namespace applog {

StringLogDevice::StringLogDevice() {
	LogWriter::Instance().attach(this);
}

StringLogDevice::~StringLogDevice() {
	LogWriter::Instance().detach(this);
}

void StringLogDevice::writeLog(const std::string& outputStr, ELogLevel level) {
	switch (level) {
		case SYS_ERROR:
			_errorStream << outputStr;
			break;
		case SYS_WARNING:
			_warningStream << outputStr;
			break;
		default:
			_logStream << outputStr;
	};
}

std::string StringLogDevice::getString(ELogLevel level)
{
	switch (level)
	{
		case SYS_ERROR:
			return  _errorStream.str();
		case SYS_WARNING:
			return  _warningStream.str();
		case SYS_STANDARD:
			return _logStream.str();
		default:
			return _logStream.str();
	};
}

void StringLogDevice::destroy() {
	InstancePtr() = StringLogDevicePtr();
}

StringLogDevicePtr& StringLogDevice::InstancePtr() {
	static StringLogDevicePtr _instance;
	return _instance;
}

} // namespace applog
