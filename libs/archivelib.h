#pragma once

#include "stream/filestream.h"
#include "stream/textfilestream.h"

/// \brief A single-byte-reader wrapper around an InputStream.
/// Optimised for reading one byte at a time.
/// Uses a buffer to reduce the number of times the wrapped stream must be read.
template<typename InputStreamType, int SIZE = 1024>
class SingleByteInputStream
{
  typedef typename InputStreamType::byte_type byte_type;

  InputStreamType& m_inputStream;
  byte_type m_buffer[SIZE];
  byte_type* m_cur;
  byte_type* m_end;

public:

  SingleByteInputStream(InputStreamType& inputStream) : m_inputStream(inputStream), m_cur(m_buffer + SIZE), m_end(m_cur)
  {
  }
  bool readByte(byte_type& b)
  {
    if(m_cur == m_end)
    {
      if(m_end != m_buffer + SIZE)
      {
        return false;
      }

      m_end = m_buffer + m_inputStream.read(m_buffer, SIZE);
      m_cur = m_buffer;

      if(m_end == m_buffer)
      {
        return false;
      }
    }

    b = *m_cur++;

    return true;
  }
};

/// \brief A binary-to-text wrapper around an InputStream.
/// Converts CRLF or LFCR line-endings to LF line-endings.
template<typename BinaryInputStreamType>
class BinaryToTextInputStream : public TextInputStream
{
  SingleByteInputStream<BinaryInputStreamType> m_inputStream;
public:
  BinaryToTextInputStream(BinaryInputStreamType& inputStream) : m_inputStream(inputStream)
  {
  }
  std::size_t read(char* buffer, std::size_t length)
  {
    char* p = buffer;
    for(;;)
    {
      if(length != 0 && m_inputStream.readByte(*reinterpret_cast<typename BinaryInputStreamType::byte_type*>(p)))
      {
        if(*p != '\r')
        {
          ++p;
          --length;
        }
      }
      else
      {
        return p - buffer;
      }
    }
  }
};
