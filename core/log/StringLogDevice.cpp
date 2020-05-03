#include "StringLogDevice.h"
#include "LogWriter.h"

namespace applog {

StringLogDevice::StringLogDevice() {
	LogWriter::Instance().attach(this);
}

StringLogDevice::~StringLogDevice() {
	LogWriter::Instance().detach(this);
}

void StringLogDevice::writeLog(const std::string& outputStr, LogLevel level)
{
	switch (level)
	{
	case LogLevel::Error:
		_errorStream << outputStr;
		break;
	case LogLevel::Warning:
		_warningStream << outputStr;
		break;
	default:
		_logStream << outputStr;
	};
}

std::string StringLogDevice::getString(LogLevel level)
{
	switch (level)
	{
	case LogLevel::Error:
		return  _errorStream.str();
	case LogLevel::Warning:
		return  _warningStream.str();
	case LogLevel::Standard:
		return _logStream.str();
	default:
		return "";
	};
}

void StringLogDevice::destroy()
{
	InstancePtr().reset();
}

StringLogDevice::Ptr& StringLogDevice::InstancePtr()
{
	static Ptr _instancePtr;
	return _instancePtr;
}

} // namespace applog
