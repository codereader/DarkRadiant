#pragma once

#include "iarchive.h"

/**
 * greebo: This wraps around a certain path in the "real"
 * filesystem on the user's hard drive.
 *
 * A real folder is treated like any other "archive" and gets
 * added to the list of PK4 archives, using this class.
 */
class DirectoryArchive final :
	public IArchive
{
	std::string _root;

	// The modname for constructing the game::IResource is cached here
	// since changing the game paths will trigger a re-initialisation
	// of the VFS anyway.
	mutable std::string _modName;

public:
	// Pass the root path to the constructor
	DirectoryArchive(const std::string& root);

	ArchiveFilePtr openFile(const std::string& name) override;
	ArchiveTextFilePtr openTextFile(const std::string& name) override;
	bool containsFile(const std::string& name) override;
	void traverse(Visitor& visitor, const std::string& root) override;

    std::size_t getFileSize(const std::string& relativePath) override;
    bool getIsPhysical(const std::string& relativePath) override;
    std::string getArchivePath(const std::string& relativePath) override;
};
typedef std::shared_ptr<DirectoryArchive> DirectoryArchivePtr;
