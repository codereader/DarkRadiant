#ifndef TEXTSTREAMBUFADAPTOR_H_
#define TEXTSTREAMBUFADAPTOR_H_

#include "itextstream.h"

#include <streambuf>

/**
 * Adapter class to implement the std::streambuf interface on an underlying
 * TextInputStream object.
 * 
 * This allows the TextInputStream objects returned from Radiant's archivezip
 * and other modules to be manipulated using familiar std::istream operations,
 * as well as allowing them to be passed to a parser::DefTokeniser without
 * first reading the entire contents into a std::string.
 * 
 * TODO: This Adapter is a stop-gap measure; ideally the lowlevel modules
 * should return std::istream subclasses directly.
 */
class TextStreambufAdaptor
: public std::streambuf
{
    // Underlying TextInputStream to read data from
    TextInputStream& _stream;
    
    // Buffer to use for reading
    static const std::size_t BUFFER_SIZE = 8192;
    char _buffer[BUFFER_SIZE];

protected:
    
    /* Implementations of stream-specific virtual functions on std::streambuf */
    
    // Replenish the controlled buffer with characters from the underlying
    // input sequence.
    int underflow() {
        std::cout << "Underflow called" << std::endl;
        
        // Read next block of BUFFER_SIZE characters into the buffer from
        // the underlying TextInputStream.
        std::size_t charsRead = _stream.read(_buffer, BUFFER_SIZE);
        
        // Set up the internal pointers correctly
        assert(charsRead <= BUFFER_SIZE);
        std::streambuf::setg(_buffer, _buffer, _buffer + charsRead);
        
        // Return the next character
        return static_cast<int>(_buffer[0]);
    }
    
public:
    
    /**
     * Constructor. Set the TextInputStream to read data from.
     */
    TextStreambufAdaptor(TextInputStream& str)
    : _stream(str)
    { 
        // Set the controlled input sequence pointers
        std::streambuf::setg(_buffer, _buffer, _buffer);
    }
    
};

#endif /*TEXTSTREAMBUFADAPTOR_H_*/
