#pragma once

#include <fstream>
#include <thread>
#include <iomanip>
#include "ilogwriter.h"
#include "itextstream.h"

namespace test
{

class TestLogFile :
    public applog::ILogDevice
{
    // The log file name including path
    std::string _logFilePath;

    // We write line by line
    std::string _buffer;

    // The file stream which will be filled with bytes
    std::ofstream _logStream;

    const char* const _timeFormat;

public:
    TestLogFile(const std::string& fullPath) :
        _logFilePath(fullPath),
        _logStream(_logFilePath.c_str(), std::ios_base::app),
        _timeFormat("%Y-%m-%d %H:%M:%S")
    {}

    // Returns true if the log stream was successfully opened
    bool isOpen()
    {
        return _logStream.good();
    }

    void writeLog(const std::string& outputStr, applog::LogLevel level) override
    {
        _buffer.append(outputStr);

        // Hold back until we hit a newline
        if (outputStr.rfind('\n') != std::string::npos)
        {
            std::time_t t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);

            // Write timestamp and thread information
            _logStream << std::put_time(&tm, _timeFormat);
            _logStream << " (" << std::this_thread::get_id() << ") ";

            // Insert the string into the stream and flush the buffer
            _logStream << _buffer;

            _buffer.clear();
            _logStream.flush();
        }
    }

    void close()
    {
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        rMessage() << std::put_time(&tm, _timeFormat) << " Closing log file." << std::endl;

        // Insert the last few remaining bytes into the stream
        if (!_buffer.empty())
        {
            _logStream << _buffer << std::endl;
            _buffer.clear();
        }

        _logStream.flush();
        _logStream.close();
    }
};

} // namespace
