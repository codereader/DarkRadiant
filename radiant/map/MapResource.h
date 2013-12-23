#pragma once

#include "imapresource.h"
#include "imapformat.h"
#include "imodel.h"
#include <set>
#include <boost/utility.hpp>
#include <boost/filesystem.hpp>

namespace map
{

class MapResource :
	public IMapResource,
	public boost::noncopyable
{
	scene::INodePtr _mapRoot;

	// Name given during construction
	std::string _originalName;

	std::string _path;
	std::string _name;

	static std::string _infoFileExt;

	// Type of resource "map"
	std::string _type;

	typedef std::set<IMapResource::Observer*> ResourceObserverList;
	ResourceObserverList _observers;

	std::time_t _modified;
	bool _realised;

public:
	// Constructor
	MapResource(const std::string& name);

	virtual ~MapResource();

	void rename(const std::string& fullPath);

	bool load();

	/**
	 * Save this resource (only for map resources).
	 *
	 * It's possible to pass a mapformat to be used for saving. If the map
	 * format argument is omitted, the format corresponding to the current
	 * game type is used.
	 *
	 * @returns
	 * true if the resource was saved, false otherwise.
	 */
	bool save(const MapFormatPtr& mapFormat = MapFormatPtr());

	// Reloads from disk
	void reload();

	scene::INodePtr getNode();
	void setNode(scene::INodePtr node);

	virtual void addObserver(Observer& observer);
	virtual void removeObserver(Observer& observer);

	bool realised();

	// Realise this MapResource
	void realise();
	void unrealise();

  std::time_t modified() const;
  void mapSave();

  bool isModified() const;
  void refresh();

	void onMapChanged();

	// Save the map contents to the given filename using the given MapFormat export module
	static bool saveFile(const MapFormat& format, const scene::INodePtr& root,
						 const GraphTraversalFunc& traverse, const std::string& filename);

private:
	// Create a backup copy of the map (used before saving)
	bool saveBackup();

	scene::INodePtr loadMapNode();

	void connectMap();

	// Returns the map format capable of loading the given stream
	// The stream pointer is guaranteed to be positioned back to the beginning
	MapFormatPtr determineMapFormat(std::istream& stream);

	bool loadFile(std::istream& mapStream, const MapFormat& format, 
				  const scene::INodePtr& root, const std::string& filename);

	// Returns a (hopefully) unique file extension for saving
	static std::string getTemporaryFileExtension();

	static bool checkIsWriteable(const boost::filesystem::path& path);
};
// Resource pointer types
typedef boost::shared_ptr<MapResource> MapResourcePtr;
typedef boost::weak_ptr<MapResource> MapResourceWeakPtr;

} // namespace map
