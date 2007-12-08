#ifndef ZIPARCHIVE_H_
#define ZIPARCHIVE_H_

#include "iarchive.h"
#include "fs_filesystem.h"
#include "stream/filestream.h"

class ZipRecord {
public:
	enum ECompressionMode
	{
		eStored,
		eDeflated,
	};
	
	ZipRecord(unsigned int position, 
			  unsigned int compressed_size,
			  unsigned int uncompressed_size, 
			  ECompressionMode mode) :
		m_position(position), 
		m_stream_size(compressed_size),
		m_file_size(uncompressed_size), 
		m_mode(mode)
	{}
	
	unsigned int m_position;
	unsigned int m_stream_size;
	unsigned int m_file_size;
	ECompressionMode m_mode;
};
typedef GenericFileSystem<ZipRecord> ZipFileSystem;

class ZipArchive : 
	public Archive
{
	ZipFileSystem m_filesystem;
	std::string m_name;
	FileInputStream m_istream;

public:
	ZipArchive(const std::string& name);
	virtual ~ZipArchive();

	bool failed();

	virtual ArchiveFilePtr openFile(const std::string& name);
	virtual ArchiveTextFilePtr openTextFile(const std::string& name);
	
	bool containsFile(const char* name);
	void forEachFile(VisitorFunc visitor, const std::string& root);
	
private:
	bool read_record();
	bool read_pkzip();
};
typedef boost::shared_ptr<ZipArchive> ZipArchivePtr;

#endif /*ZIPARCHIVE_H_*/
