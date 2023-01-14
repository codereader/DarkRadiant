#pragma once

#include <vector>
#include "iarchive.h"
#include "ifilesystem.h"

namespace vfs
{

class AssetsList;

class Doom3FileSystem :
	public VirtualFileSystem
{
private:
	// Our ordered list of paths to search
	SearchPaths _vfsSearchPaths;

	std::vector<std::string> _directories;
    std::set<std::string> _allowedExtensions;
	std::set<std::string> _allowedExtensionsDir;

	struct ArchiveDescriptor
	{
		std::string name;
		IArchive::Ptr archive;
		bool is_pakfile;
	};

    std::list<ArchiveDescriptor> _archives;

    sigc::signal<void> _sigInitialised;

public:
	void initialise(const SearchPaths& vfsSearchPaths, const std::set<std::string>& allowedExtensions) override;
    bool isInitialised() const override;
	void shutdown() override;

    const std::set<std::string>& getArchiveExtensions() const override;

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

    sigc::signal<void>& signal_Initialised() override;

	const SearchPaths& getVfsSearchPaths() override;
    FileInfo getFileInfo(const std::string& vfsRelativePath) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void initDirectory(const std::string& path);
	void initPakFile(const std::string& filename);

    std::shared_ptr<AssetsList> findAssetsList(const std::string& topLevelPath);
};

}
