#ifndef MAPRESOURCE_H_
#define MAPRESOURCE_H_

#include "imapresource.h"
#include "imap.h"
#include "imodel.h"
#include "generic/callback.h"
#include <set>
#include <boost/utility.hpp>

namespace map {

	namespace {
		const std::string RKEY_INFO_FILE_EXTENSION("game/mapFormat/infoFileExtension");
	}

class MapResource : 
	public IMapResource,
	public boost::noncopyable
{
	scene::INodePtr _mapRoot;
  
	// Name given during construction
	std::string _originalName;
	
	std::string _path;
	std::string _name;
  
	// Type of resource (map, lwo etc)
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
	 * @returns
	 * true if the resource was saved, false otherwise.
	 */
	bool save();
	
	void flush();
	
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
	typedef MemberCaller<MapResource, &MapResource::onMapChanged> MapChangedCaller;
	
private:
	// Create a backup copy of the map (used before saving)
	bool saveBackup();
	
	scene::INodePtr loadMapNode();
	
	void connectMap();
	
	MapFormatPtr getMapFormat();

	bool loadFile(const MapFormat& format, scene::INodePtr root, const std::string& filename);
};
// Resource pointer types
typedef boost::shared_ptr<MapResource> MapResourcePtr;
typedef boost::weak_ptr<MapResource> MapResourceWeakPtr;

} // namespace map

#endif /*MAPRESOURCE_H_*/
