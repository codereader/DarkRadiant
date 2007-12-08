#ifndef DEFLATEDARCHIVEFILE_H_
#define DEFLATEDARCHIVEFILE_H_

#include "iarchive.h"
#include "stream/filestream.h"

class DeflatedArchiveFile : 
	public ArchiveFile
{
	std::string m_name;
	FileInputStream m_istream;
	SubFileInputStream m_substream;
	DeflatedInputStream m_zipstream;
	FileInputStream::size_type m_size;
public:
	typedef FileInputStream::size_type size_type;
	typedef FileInputStream::position_type position_type;

	DeflatedArchiveFile(const std::string& name, 
						const std::string& archiveName, 
						position_type position, 
						size_type stream_size, 
						size_type file_size) : 
		m_name(name), 
		m_istream(archiveName), 
		m_substream(m_istream, position, stream_size), 
		m_zipstream(m_substream), m_size(file_size)
	{}

	size_type size() const {
		return m_size;
	}
	
	const std::string& getName() const {
		return m_name;
	}
	
	InputStream& getInputStream() {
		return m_zipstream;
	}
};

#endif /*DEFLATEDARCHIVEFILE_H_*/
