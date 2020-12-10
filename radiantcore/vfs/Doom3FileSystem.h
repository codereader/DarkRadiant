#pragma once

#include "iarchive.h"
#include "ifilesystem.h"

namespace vfs
{

class Doom3FileSystem :
	public VirtualFileSystem
{
private:
	// Our ordered list of paths to search
	SearchPaths _vfsSearchPaths;

	std::list<std::string> _directories;
	ExtensionSet _allowedExtensions;
	std::set<std::string> _allowedExtensionsDir;

	struct ArchiveDescriptor
	{
		std::string name;
		IArchive::Ptr archive;
		bool is_pakfile;
	};

	typedef std::list<ArchiveDescriptor> ArchiveList;
	ArchiveList _archives;

	typedef std::set<Observer*> ObserverList;
	ObserverList _observers;

public:
	void initialise(const SearchPaths& vfsSearchPaths, const ExtensionSet& allowedExtensions) override;
    bool isInitialised() const override;
	void shutdown() override;

    const ExtensionSet& getArchiveExtensions() const override;

	int getFileCount(const std::string& filename) override;
	ArchiveFilePtr openFile(const std::string& filename) override;
	ArchiveTextFilePtr openTextFile(const std::string& filename) override;

	ArchiveFilePtr openFileInAbsolutePath(const std::string& filename) override;
	ArchiveTextFilePtr openTextFileInAbsolutePath(const std::string& filename) override;
    IArchive::Ptr openArchiveInAbsolutePath(const std::string& pathToArchive) override;

	// Call the specified callback function for each file matching extension
	// inside basedir.
	void forEachFile(const std::string& basedir, const std::string& extension,
		const VisitorFunc& visitorFunc, std::size_t depth) override;

	void forEachFileInAbsolutePath(const std::string& path,
		const std::string& extension,
		const VisitorFunc& visitorFunc,
		std::size_t depth = 1) override;

    void forEachFileInArchive(const std::string& absoluteArchivePath,
        const std::string& extension,
        const VisitorFunc& visitorFunc,
        std::size_t depth = 1) override;

	std::string findFile(const std::string& name) override;
	std::string findRoot(const std::string& name) override;

	void addObserver(Observer& observer) override;
	void removeObserver(Observer& observer) override;

	const SearchPaths& getVfsSearchPaths() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void initDirectory(const std::string& path);
	void initPakFile(const std::string& filename);
};

}
