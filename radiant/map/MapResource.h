#pragma once

#include "imapresource.h"
#include "imapformat.h"
#include "imapinfofile.h"
#include "imodel.h"
#include "imap.h"
#include <set>
#include "RootNode.h"
#include "os/fs.h"

namespace map
{

class MapResource :
	public IMapResource,
	public util::Noncopyable
{
private:
    RootNodePtr _mapRoot;

	// Name given during construction
	std::string _originalName;

	std::string _path;
	std::string _name;

	static std::string _infoFileExt;

	// Type of resource "map"
	std::string _type;

public:
	// Constructor
	MapResource(const std::string& name);

	void rename(const std::string& fullPath) override;

	bool load() override;
	bool save(const MapFormatPtr& mapFormat = MapFormatPtr()) override;

	scene::IMapRootNodePtr getNode() override;
    void setNode(const scene::IMapRootNodePtr& node) override;

	// Save the map contents to the given filename using the given MapFormat export module
	static bool saveFile(const MapFormat& format, const scene::INodePtr& root,
						 const GraphTraversalFunc& traverse, const std::string& filename);

private:
	void mapSave();
	void onMapChanged();

	// Create a backup copy of the map (used before saving)
	bool saveBackup();

	RootNodePtr loadMapNode();
    RootNodePtr loadMapNodeFromStream(std::istream& stream, const std::string& fullPath);

	void connectMap();

	// Returns the map format capable of loading the given stream
	// The stream pointer is guaranteed to be positioned back to the beginning
	MapFormatPtr determineMapFormat(std::istream& stream);

	bool loadFile(std::istream& mapStream, const MapFormat& format, 
                  const RootNodePtr& root, const std::string& filename);

	void loadInfoFile(const RootNodePtr& root, const std::string& filename, const NodeIndexMap& nodeMap);
	void loadInfoFileFromStream(std::istream& infoFileStream, const RootNodePtr& root, const NodeIndexMap& nodeMap);

	// Opens a stream for the given path, which might be VFS path or an absolute one. The streamProcessor
	// function is then called with the opened stream. Throws std::runtime_error on stream open failure.
	void openFileStream(const std::string& path, const std::function<void(std::istream&)>& streamProcessor);

	static bool checkIsWriteable(const fs::path& path);
};
// Resource pointer types
typedef std::shared_ptr<MapResource> MapResourcePtr;
typedef std::weak_ptr<MapResource> MapResourceWeakPtr;

} // namespace map
