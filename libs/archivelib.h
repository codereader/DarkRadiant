#pragma once

#include "debugging/debugging.h"
#include "iarchive.h"
#include "stream/filestream.h"
#include "stream/textfilestream.h"
#include "string/string.h"
#include "os/path.h"
#include "gamelib.h"

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

/// \brief An ArchiveFile which is stored uncompressed as part of a larger archive file.
class StoredArchiveFile :
	public ArchiveFile
{
	std::string m_name;
	FileInputStream m_filestream;
	SubFileInputStream m_substream;
	FileInputStream::size_type m_size;
public:
	typedef FileInputStream::size_type size_type;
	typedef FileInputStream::position_type position_type;

	StoredArchiveFile(const std::string& name,
					const std::string& archiveName,
					position_type position,
					size_type stream_size,
					size_type file_size)
		: m_name(name),
		  m_filestream(archiveName),
		  m_substream(m_filestream, position, stream_size),
		  m_size(file_size)
	{}

	size_type size() const {
		return m_size;
	}

	const std::string& getName() const {
		return m_name;
	}

	InputStream& getInputStream() {
		return m_substream;
	}
};

/// \brief An ArchiveTextFile which is stored uncompressed as part of a larger archive file.
class StoredArchiveTextFile :
	public ArchiveTextFile
{
	std::string m_name;
	FileInputStream m_filestream;
	SubFileInputStream m_substream;
	BinaryToTextInputStream<SubFileInputStream> m_textStream;

    // Mod directory
    std::string _modDir;
public:
	typedef FileInputStream::size_type size_type;
	typedef FileInputStream::position_type position_type;

    /**
     * Constructor.
     *
     * @param modDir
     * Name of the mod directory containing this file.
     */
    StoredArchiveTextFile(const std::string& name,
                          const std::string& archiveName,
                          const std::string& modDir,
                          position_type position,
                          size_type stream_size)
    : m_name(name),
      m_filestream(archiveName),
      m_substream(m_filestream, position, stream_size),
      m_textStream(m_substream),
	  _modDir(game::current::getModPath(modDir))
    {}

 	const std::string& getName() const {
		return m_name;
	}

	TextInputStream& getInputStream() {
		return m_textStream;
	}

    /**
     * Return mod directory.
     */
    std::string getModName() const {
        return _modDir;
    }
};
typedef std::shared_ptr<StoredArchiveTextFile> StoredArchiveTextFilePtr;

/// \brief An ArchiveFile which is stored as a single file on disk.
class DirectoryArchiveFile :
	public ArchiveFile
{
	std::string m_name;
	FileInputStream m_istream;
	FileInputStream::size_type m_size;
public:
	typedef FileInputStream::size_type size_type;

	DirectoryArchiveFile(const std::string& name, const std::string& filename) :
		m_name(name),
		m_istream(filename)
	{
		if (!failed()) {
			m_istream.seek(0, FileInputStream::end);
  			m_size = m_istream.tell();
  			m_istream.seek(0);
      	}
		else {
			m_size = 0;
		}
	}

	bool failed() const {
		return m_istream.failed();
	}

	size_type size() const {
		return m_size;
	}

	const std::string& getName() const {
		return m_name;
	}

	InputStream& getInputStream() {
		return m_istream;
	}
};
typedef std::shared_ptr<DirectoryArchiveFile> DirectoryArchiveFilePtr;

/// \brief An ArchiveTextFile which is stored as a single file on disk.
class DirectoryArchiveTextFile :
	public ArchiveTextFile
{
	std::string m_name;
	TextFileInputStream m_inputStream;

    // Mod directory
    std::string _modDir;
public:

    DirectoryArchiveTextFile(const std::string& name,
                             const std::string& modDir,
                             const std::string& filename)
    : m_name(name),
      m_inputStream(filename.c_str()),
      _modDir(game::current::getModPath(modDir))
    {}

	bool failed() const {
		return m_inputStream.failed();
	}

	const std::string& getName() const {
		return m_name;
	}

	TextInputStream& getInputStream() {
		return m_inputStream;
	}

    /**
     * Get mod directory.
     */
    std::string getModName() const {
        return _modDir;
    }
};
typedef std::shared_ptr<DirectoryArchiveTextFile> DirectoryArchiveTextFilePtr;
