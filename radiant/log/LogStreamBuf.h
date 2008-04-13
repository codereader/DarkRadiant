#ifndef _LOG_STREAM_BUF_H_
#define _LOG_STREAM_BUF_H_

#include <streambuf>

namespace applog {

/**
 * greebo: The LogStreamBuf adapts the std::streambuf to use to the
 *         LogWriter class for the actual logging.
 */
class LogStreamBuf : 
	public std::streambuf
{
	// Internal character buffer
	char* _reserve;

	int _level;

public:
	/**
	 * greebo: Pass the level and the optional buffersize to the constructor.
	 *         Level can be something like SYS_ERR, SYS_STD, etc.
	 */
	LogStreamBuf(int level, int bufferSize = 1);

	virtual ~LogStreamBuf();

protected:
	// These two get called by the base class streambuf
	virtual int_type overflow(int_type c);

	virtual int_type sync();

private:
	void writeToBuffer();
};

} // namespace applog

#endif /* _LOG_STREAM_BUF_H_ */
