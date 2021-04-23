#pragma once

#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include "string/replace.h"
#include "ilogwriter.h"

namespace test
{

/**
 * The ConsoleLogger writes all messages to the std output streams
 */
class ConsoleLogger :
	public applog::ILogDevice
{
private:
    applog::LogLevel _currentLevel;
	std::string _buffer;

public:
	/**
	 * greebo: This method gets called by the Writer with
	 * a logging string as argument.
	 */
	void writeLog(const std::string& outputStr, applog::LogLevel level) override
    {
        // The text usually arrives in single characters at a time
        // In case the level changes, we need to flush the line
        if (_currentLevel != level)
        {
            flushLine();
        }

        // Write to the buffer first
        _currentLevel = level;
        _buffer.append(outputStr);

        // Once we hit a single newline, flush the line
        if (outputStr == "\n")
        {
            flushLine();
        }
    }

private:
    void flushLine()
    {
        if (!_buffer.empty())
        {
            // Replace NULL characters
            string::replace_all(_buffer, std::string(1, '\0'), "NULL");

            switch (_currentLevel)
            {
            case applog::LogLevel::Verbose:
                std::cout << "[VRB]: " << _buffer;
                break;
            case applog::LogLevel::Standard:
                std::cout << "[STD]: " << _buffer;
                break;
            case applog::LogLevel::Warning:
                std::cout << "[WRN]: " << _buffer;
                break;
            case applog::LogLevel::Error:
                std::cerr << "[ERR]: " << _buffer;
                break;
            default:
                std::cout << "[???]: " << _buffer;
            };

            _buffer.clear();
        }
    }
};

} // namespace applog
