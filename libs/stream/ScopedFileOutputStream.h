#pragma once

#include <memory>
#include <fstream>
#include <string>
#include <cassert>
#include "util/Noncopyable.h"

namespace stream
{

/**
 * Helper class wrapping around a std::ofstream. The instance can be empty,
 * until a filename is been passed to it.
 *
 * If there is a contained stream, it is ensured that it is 
 * closed when the instance is destroyed.
 * 
 * The underlying stream can be accessed through the dereference operator*()
 */
class ScopedOutputStream :
    public std::ofstream
{
private:
    std::unique_ptr<std::ofstream> _stream;

public:
    // Constructs an empty stream object, no stream is opened
    // until the open() method is invoked
    ScopedFileOutputStream()
    {}

    // Constructs the wrapper and opens the internal stream
    // using the given filename
    ScopedFileOutputStream(const std::string& filename)
	{
        open(filename);
    }

    ~ScopedFileOutputStream()
    {
        if (_stream)
        {
            _stream->flush();
            _stream->close();
            _stream.reset();
        }
    }

    // Returns true if there's no underlying stream object instantiated
    bool isEmpty() const
    {
        return static_cast<bool>(_stream);
    }

    bool isOpen() const
    {
        return _stream && _stream->is_open();
    }

    std::ostream& operator*()
    {
        assert(_stream);
        return *_stream;
    }

    const std::ostream& operator*() const
    {
        assert(_stream);
        return *_stream;
    }

    // Open the internal stream using the given filename
    void open(const std::string& filename)
    {
        assert(!_stream); // no duplicate open calls

        _stream.reset(new std::ofstream(filename));
    }
};

}
