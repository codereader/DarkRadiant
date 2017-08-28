#pragma once

#include "iarchive.h"
#include "GenericFileSystem.h"
#include "stream/filestream.h"
#include <mutex>

namespace archive
{

class ZipArchive :
	public Archive
{
private:
	class ZipRecord
	{
	public:
		enum CompressionMode
		{
			eStored,
			eDeflated,
		};

		ZipRecord(unsigned int position_,
				  unsigned int compressed_size_,
				  unsigned int uncompressed_size_,
				  CompressionMode mode_) :
			position(position_),
			stream_size(compressed_size_),
			file_size(uncompressed_size_),
			mode(mode_)
		{}

		unsigned int position;
		unsigned int stream_size;
		unsigned int file_size;
		CompressionMode mode;
	};
	typedef GenericFileSystem<ZipRecord> ZipFileSystem;

	ZipFileSystem _filesystem;
	std::string _fullPath;			// the full path to the Zip file
	std::string _containingFolder;  // the folder this Zip is located in
	FileInputStream _istream;
    std::mutex _streamLock;

public:
	ZipArchive(const std::string& fullPath);
	virtual ~ZipArchive();

	virtual ArchiveFilePtr openFile(const std::string& name) override;
	virtual ArchiveTextFilePtr openTextFile(const std::string& name) override;
	bool containsFile(const std::string& name) override;
	void forEachFile(VisitorFunc visitor, const std::string& root) override;

private:
	bool read_record();
	bool loadZipFile();
};

}
