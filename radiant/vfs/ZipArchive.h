#pragma once

#include "iarchive.h"
#include "GenericFileSystem.h"
#include "stream/FileInputStream.h"
#include <mutex>

namespace archive
{

/**
 * Archive adapter representing a PK4 (Zip) archive file.
 *
 * In idTech4 Virtual Filesystems this is the only other 
 * Archive type allowed, next to the DirectoryArchives representing 
 * physical directories.
 *
 * Archives are owned and instantiated by the GlobalFileSystem instance.
 */
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

		ZipRecord(uint32_t position_,
				  uint32_t compressed_size_,
				  uint32_t uncompressed_size_,
				  CompressionMode mode_) :
			position(position_),
			stream_size(compressed_size_),
			file_size(uncompressed_size_),
			mode(mode_)
		{}

		uint32_t position;
		uint32_t stream_size;
		uint32_t file_size;
		CompressionMode mode;
	};
	typedef GenericFileSystem<ZipRecord> ZipFileSystem;

	ZipFileSystem _filesystem;
	std::string _fullPath;			// the full path to the Zip file
	std::string _containingFolder;  // the folder this Zip is located in
	mutable std::string _modName;	// mod name, calculated based on the containing folder
	stream::FileInputStream _istream;
    std::mutex _streamLock;

public:
	ZipArchive(const std::string& fullPath);
	virtual ~ZipArchive();

	// Archive implementation
	virtual ArchiveFilePtr openFile(const std::string& name) override;
	virtual ArchiveTextFilePtr openTextFile(const std::string& name) override;
	bool containsFile(const std::string& name) override;
	void traverse(Visitor& visitor, const std::string& root) override;

private:
	void readZipRecord();
	void loadZipFile();
};

}
