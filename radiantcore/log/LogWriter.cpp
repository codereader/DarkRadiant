#include "LogWriter.h"

#include <cassert>
#include <tuple>
#include "StringLogDevice.h"

namespace applog
{

LogWriter::LogWriter()
{
	for (auto level : AllLogLevels)
	{
		_streams.emplace(level, level);
	}
}

void LogWriter::write(const char* p, std::size_t length, LogLevel level)
{
	// Convert the buffer to a string
	std::string output(p, length);

	// Visit all the logfiles and write the string
	for (auto device : _devices)
    {
		device->writeLog(output, level);
	}
}

std::ostream& LogWriter::getLogStream(LogLevel level)
{
	assert(_streams.find(level) != _streams.end());
	return _streams.at(level);
}

std::mutex& LogWriter::getStreamLock()
{
	return LogStream::GetStreamLock();
}

void LogWriter::attach(ILogDevice* device)
{
	_devices.insert(device);

	if (device->isConsole())
	{
		// The first console device receives all the buffered output
		if (applog::StringLogDevice::InstancePtr())
		{
			applog::StringLogDevice& logger = *applog::StringLogDevice::InstancePtr();

			for (auto level : applog::AllLogLevels)
			{
				std::string bufferedText = logger.getString(static_cast<applog::LogLevel>(level));

				if (bufferedText.empty()) continue;

				device->writeLog(bufferedText + "\n", static_cast<applog::LogLevel>(level));
			}
		}

		applog::StringLogDevice::destroy();
	}
}

void LogWriter::detach(ILogDevice* device)
{
	_devices.erase(device);
}

LogWriter& LogWriter::Instance()
{
	static LogWriter _writer;
	return _writer;
}

} // namespace applog
