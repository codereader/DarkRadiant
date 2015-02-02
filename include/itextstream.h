#pragma once

/// \file
/// \brief Text-stream interfaces.

#include <cstddef>
#include <string>
#include <stdio.h>
#include <streambuf>
#include <istream>
#include <cassert>
#include <sstream>
#include <iostream>
#include <mutex>

#include "imodule.h"

/// \brief A read-only character-stream.
// OrbWeaver: merged functionality from TextStreambufAdaptor onto this class
// directly.
class TextInputStream
: public std::streambuf
{
protected:
    // Buffer to use for reading
    static const std::size_t BUFFER_SIZE = 8192;
    char _buffer[BUFFER_SIZE];

protected:

    /* Implementations of stream-specific virtual functions on std::streambuf */

    // Replenish the controlled buffer with characters from the underlying
    // input sequence.
    virtual int underflow()
    {
        // Read next block of BUFFER_SIZE characters into the buffer from
        // the underlying TextInputStream.
        std::size_t charsRead = this->read(_buffer, BUFFER_SIZE);

        // Set up the internal pointers correctly
        assert(charsRead <= BUFFER_SIZE);
        std::streambuf::setg(_buffer, _buffer, _buffer + charsRead);

        // Return the next character, or EOF if there were no more characters
        if (charsRead > 0)
        	return static_cast<int>(_buffer[0]);
        else
        	return EOF;
    }

public:

	/// \brief Attempts to read the next \p length characters from the stream to \p buffer.
	/// Returns the number of characters actually stored in \p buffer.
	virtual std::size_t read(char* buffer, std::size_t length) = 0;

};

class NullOutputBuf :
	public std::streambuf
{
protected:
	virtual std::size_t xsputn(const char*, std::size_t len) {
		// Override the virtual xsputn method to do nothing instead
		return len;
	}
};

class NullOutputStream :
	public std::ostream
{
	NullOutputBuf _nullBuf;
public:
	NullOutputStream() :
		std::ostream(&_nullBuf)
	{}
};

/**
 * greebo: This is a simple container holding a single output stream.
 * Use the getStream() method to acquire a reference to the stream.
 */
class OutputStreamHolder
{
	NullOutputStream _nullOutputStream;
    std::mutex _nullLock;
	std::ostream* _outputStream;
    std::mutex* _streamLock;

public:
	OutputStreamHolder() :
		_outputStream(&_nullOutputStream),
        _streamLock(&_nullLock)
	{}

	void setStream(std::ostream& outputStream) {
		_outputStream = &outputStream;
	}

	std::ostream& getStream() {
		return *_outputStream;
	}

    void setLock(std::mutex& streamLock)
    {
        _streamLock = &streamLock;
    }

    std::mutex& getStreamLock()
    {
        return *_streamLock;
    }
};

// With multiple threads writing against a single thread-unsafe std::ostream
// we need to buffer the stream and write it to the underlying stream
// in a thread-safe manner. The TemporaryThreadsafeStream passes its data
// in the destructor - since std::ostringstream doesn't define a virtual
// destructor client code should not cast the stream reference to its base
// std::stringstream otherwise the destructor might not be called.
class TemporaryThreadsafeStream :
    public std::ostringstream
{
private:
    std::ostream& _actualStream;
    std::mutex& _streamLock;

public:
    TemporaryThreadsafeStream(std::ostream& actualStream, std::mutex& streamLock) :
        _actualStream(actualStream),
        _streamLock(streamLock)
    {
        _actualStream.copyfmt(*this);
        setstate(actualStream.rdstate());
    }

    // On destruction, we flush our buffer to the main stream
    // in a thread-safe manner
    ~TemporaryThreadsafeStream()
    {
        std::lock_guard<std::mutex> lock(_streamLock);

        // Flush buffer on destruction
        _actualStream << str();
    }
};


// The static stream holder containers, these are instantiated by each
// module (DLL/so) at the time of the first call.
inline OutputStreamHolder& GlobalOutputStream()
{
	static OutputStreamHolder _holder;
	return _holder;
}

inline OutputStreamHolder& GlobalErrorStream()
{
	static OutputStreamHolder _holder;
	return _holder;
}

inline OutputStreamHolder& GlobalWarningStream()
{
	static OutputStreamHolder _holder;
	return _holder;
}

inline OutputStreamHolder& GlobalDebugStream()
{
	static OutputStreamHolder _holder;
	return _holder;
}

// The stream accessors: use these to write to the application's various streams.
// Note that the TemporaryThreadsafeStream is using the SAME std::mutex (on purpose)
// to avoid error and debug streams from concurrently writing to the same log device.
inline TemporaryThreadsafeStream rMessage()
{
    return TemporaryThreadsafeStream(
        GlobalOutputStream().getStream(), 
        GlobalOutputStream().getStreamLock()
    );
}

inline TemporaryThreadsafeStream rError()
{
    return TemporaryThreadsafeStream(
        GlobalErrorStream().getStream(),
        GlobalErrorStream().getStreamLock()
    );
}

inline TemporaryThreadsafeStream rWarning()
{
    return TemporaryThreadsafeStream(
        GlobalWarningStream().getStream(),
        GlobalWarningStream().getStreamLock()
    );
}

/**
 * \brief
 * Get the debug output stream.
 *
 * In debug builds the debug stream is the same as the output stream. In release
 * builds it is a null stream.
 */
inline TemporaryThreadsafeStream rDebug()
{
    return TemporaryThreadsafeStream(
        GlobalDebugStream().getStream(),
        GlobalDebugStream().getStreamLock()
    );
}

namespace module
{

// greebo: This is called once by each module at load time to initialise
// the OutputStreamHolders above.
inline void initialiseStreams(const ApplicationContext& ctx)
{
	GlobalOutputStream().setStream(ctx.getOutputStream());
	GlobalWarningStream().setStream(ctx.getWarningStream());
	GlobalErrorStream().setStream(ctx.getErrorStream());

#ifndef NDEBUG
    GlobalDebugStream().setStream(ctx.getOutputStream());
#endif

    // Set up the mutex for thread-safe logging
    GlobalOutputStream().setLock(ctx.getStreamLock());
    GlobalWarningStream().setLock(ctx.getStreamLock());
    GlobalErrorStream().setLock(ctx.getStreamLock());
    GlobalDebugStream().setLock(ctx.getStreamLock());
}

} // namespace module
