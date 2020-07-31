#include "LogFile.h"

#include <iomanip>
#include <thread>
#include "imodule.h"
#include "itextstream.h"
#include "version.h"

#include "string/convert.h"

namespace applog
{

namespace
{
    const char* const TIME_FMT = "%Y-%m-%d %H:%M:%S";
}

LogFile::LogFile(const std::string& fullPath) :
	_logFilePath(fullPath),
    _logStream(_logFilePath.c_str())
{}

bool LogFile::isOpen()
{
    return _logStream.good();
}

const std::string& LogFile::getFullPath() const
{
    return _logFilePath;
}

void LogFile::writeLog(const std::string& outputStr, LogLevel level) 
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

void LogFile::close()
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
}

} // namespace applog
