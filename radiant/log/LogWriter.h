#ifndef _LOG_WRITER_H_
#define _LOG_WRITER_H_

#include <cstddef>

typedef struct _GtkTextBuffer GtkTextBuffer;
typedef struct _GtkTextTag GtkTextTag;

namespace applog {

/**
 * greebo: These are the various warning levels. They are mostly used
 *         to "tag" the according output with various colours in the console.
 */
enum ELogLevel {
	SYS_VERBOSE = 0,
	SYS_STANDARD = 1,
	SYS_WARNING = 2,
	SYS_ERROR = 3,
};

class LogWriter
{
	GtkTextBuffer* _buffer;
	
	// The tags needed for the console output
	static GtkTextTag* errorTag;
	static GtkTextTag* warningTag;
	static GtkTextTag* standardTag;

public:
	LogWriter();

	void write(const char* p, std::size_t length, int level);

	void disconnectConsoleWindow();

	static LogWriter& Instance();
};

} // namespace applog

#endif /* _LOG_WRITER_H_ */
