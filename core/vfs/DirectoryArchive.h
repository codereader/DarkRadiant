#pragma once

#include "Archive.h"

/**
 * greebo: This wraps around a certain path in the "real"
 * filesystem on the user's hard drive.
 *
 * A real folder is treated like any other "archive" and gets
 * added to the list of PK4 archives, using this class.
 */
class DirectoryArchive :
	public Archive
{
	std::string _root;

	// The modname for constructing the ModResource is cached here
	// since changing the game paths will trigger a re-initialisation
	// of the VFS anyway.
	mutable std::string _modName;

public:
	// Pass the root path to the constructor
	DirectoryArchive(const std::string& root);

	virtual ArchiveFilePtr openFile(const std::string& name) override;

	virtual ArchiveTextFilePtr openTextFile(const std::string& name) override;

	virtual bool containsFile(const std::string& name) override;

	virtual void traverse(Visitor& visitor, const std::string& root) override;
};
typedef std::shared_ptr<DirectoryArchive> DirectoryArchivePtr;
