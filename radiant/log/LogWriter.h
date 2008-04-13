#ifndef _LOG_WRITER_H_
#define _LOG_WRITER_H_

#include <cstddef>
#include "LogLevels.h"

namespace applog {

/**
 * greebo: These are the various warning levels. They are mostly used
 *         to "tag" the according output with various colours in the console.

 * TODO: Make Console and Logfile derive from an abstract LogDevice class, attached to this Writer.
 */
class LogWriter
{
public:
	/** 
	 * greebo: Writes the given buffer p with the given length to the 
	 *         various output devices (i.e. Console and Log file).
	 */
	void write(const char* p, std::size_t length, ELogLevel level);

	// Contains the static singleton instance of this writer
	static LogWriter& Instance();
};

} // namespace applog

#endif /* _LOG_WRITER_H_ */
