#pragma once

#include "imapresource.h"
#include "imapformat.h"
#include "imodel.h"
#include "imap.h"
#include <set>
#include "RootNode.h"
#include "os/fs.h"
#include "stream/MapResourceStream.h"

namespace map
{

class MapResource :
	public IMapResource,
	public util::Noncopyable
{
private:
    scene::IMapRootNodePtr _mapRoot;

    // Contains the absolute base path (e.g. c:/games/darkmod/) if the resourcePath
    // points to a directory which is part of the VFS search paths. 
    // Will be an empty string if the resource is pointing to a path outside the VFS.
	std::string _path;

    // Either contains the path relative to the base path (e.g. "maps/arkham.map")
    // or the full absolute path to the map (in case the resource path 
    // is pointing to a path outside the VFS)
	std::string _name;

	// File extension of this resource
	std::string _extension;

    // The modification time this resource had when it was loaded
    // This is used to protect accidental overwrites of files that
    // have been modified since the last load time
    fs::file_time_type _lastKnownModificationTime;

public:
	// Constructor
	MapResource(const std::string& resourcePath);
    virtual ~MapResource();

	virtual void rename(const std::string& fullPath) override;

	virtual bool load() override;
    virtual bool isReadOnly() override;
	virtual void save(const MapFormatPtr& mapFormat = MapFormatPtr()) override;

	virtual const scene::IMapRootNodePtr& getRootNode() override;
    virtual void setRootNode(const scene::IMapRootNodePtr& root) override;
    virtual void clear() override;

    // Check if the file has been modified since it was last saved
    virtual bool fileHasBeenModifiedSinceLastSave() override;

	// Save the map contents to the given filename using the given MapFormat export module
	// Throws an OperationException if anything prevents successful completion
	static void saveFile(const MapFormat& format, const scene::IMapRootNodePtr& root,
						 const GraphTraversalFunc& traverse, const std::string& filename);

protected:
    // Implementation-specific method to open the stream of the primary .map or .mapx file
    // May return an empty reference, may throw OperationException on failure
    virtual stream::MapResourceStream::Ptr openMapfileStream();

    // Implementation-specific method to open the info file stream (.darkradiant) file
    // May return an empty reference, may throw OperationException on failure
    virtual stream::MapResourceStream::Ptr openInfofileStream();

    // Returns true if the file can be written to. Also returns true if the file
    // doesn't exist (assuming the file can always be created).
    static bool FileIsWriteable(const fs::path& path);

private:
    void constructPaths(const std::string& resourcePath);
    std::string getAbsoluteResourcePath();

    void refreshLastModifiedTime();
	void mapSave();
	void onMapChanged();

	// Create a backup copy of the map (used before saving)
	bool saveBackup();

	RootNodePtr loadMapNode();

	// Opens a stream for the given path, which might be VFS path or an absolute one. 
    // Throws IMapResource::OperationException on stream open failure.
	stream::MapResourceStream::Ptr openFileStream(const std::string& path);

	// Checks if file can be overwritten (throws on failure)
	static void throwIfNotWriteable(const fs::path& path);
};

} // namespace map
