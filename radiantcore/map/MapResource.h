#pragma once

#include "imapresource.h"
#include "imapformat.h"
#include "imapinfofile.h"
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

public:
	// Constructor
	MapResource(const std::string& resourcePath);

	void rename(const std::string& fullPath) override;

	bool load() override;
	void save(const MapFormatPtr& mapFormat = MapFormatPtr()) override;

	const scene::IMapRootNodePtr& getRootNode() override;
    void clear() override;

	// Save the map contents to the given filename using the given MapFormat export module
	// Throws an OperationException if anything prevents successful completion
	static void saveFile(const MapFormat& format, const scene::IMapRootNodePtr& root,
						 const GraphTraversalFunc& traverse, const std::string& filename);

private:
    void constructPaths(const std::string& resourcePath);
    std::string getAbsoluteResourcePath();

	void mapSave();
	void onMapChanged();

	// Create a backup copy of the map (used before saving)
	bool saveBackup();

	RootNodePtr loadMapNode();

	void connectMap();

	// Opens a stream for the given path, which might be VFS path or an absolute one. 
    // Throws IMapResource::OperationException on stream open failure.
	stream::MapResourceStream::Ptr openFileStream(const std::string& path);

    // Returns the extension of the auxiliary info file (including the leading dot character)
    static std::string getInfoFileExtension();

	// Checks if file can be overwritten (throws on failure)
	static void throwIfNotWriteable(const fs::path& path);
};
// Resource pointer types
typedef std::shared_ptr<MapResource> MapResourcePtr;
typedef std::weak_ptr<MapResource> MapResourceWeakPtr;

} // namespace map
