#pragma once

#include <list>
#include "iarchive.h"
#include "ifilesystem.h"

#define VFS_MAXDIRS 8

class Doom3FileSystem :
	public VirtualFileSystem
{
	std::string _directories[VFS_MAXDIRS];
	int _numDirectories;
	std::set<std::string> _allowedExtensions;
	std::set<std::string> _allowedExtensionsDir;

	struct ArchiveDescriptor {
		std::string name;
		ArchivePtr archive;
		bool is_pakfile;
	};

	typedef std::list<ArchiveDescriptor> ArchiveList;
	ArchiveList _archives;

	typedef std::set<Observer*> ObserverList;
	ObserverList _observers;

public:
	// Constructor
	Doom3FileSystem();

	void initDirectory(const std::string& path);
	void initialise();
	void shutdown();

	int getFileCount(const std::string& filename);
	ArchiveFilePtr openFile(const std::string& filename);
	ArchiveTextFilePtr openTextFile(const std::string& filename);

	std::size_t loadFile(const std::string& filename, void **buffer);
	void freeFile(void *p);

	// Call the specified callback function for each file matching extension
	// inside basedir.
    void forEachFile(const std::string& basedir, const std::string& extension,
                     const VisitorFunc& visitorFunc, std::size_t depth);

	std::string findFile(const std::string& name);
	std::string findRoot(const std::string& name);

	virtual void addObserver(Observer& observer);
	virtual void removeObserver(Observer& observer);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:
	void initPakFile(ArchiveLoader& archiveModule, const std::string& filename);
};
typedef boost::shared_ptr<Doom3FileSystem> Doom3FileSystemPtr;
