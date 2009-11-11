#ifndef _IMAPRESOURCE_H__
#define _IMAPRESOURCE_H__

#include "inode.h"
#include "imodule.h"

class IMapResource
{
public:
	class Observer {
	public:
	    virtual ~Observer() {}
		virtual void onResourceRealise() = 0;
		virtual void onResourceUnrealise() = 0;
	};

	virtual ~IMapResource() {}

	// Renames this map resource to the new path
	virtual void rename(const std::string& fullPath) = 0;
	
	virtual bool load() = 0;
	virtual bool save() = 0;

	// Reloads the map file from disk
	virtual void reload() = 0;

	virtual scene::INodePtr getNode() = 0;
	virtual void setNode(scene::INodePtr node) = 0;
	
	virtual void addObserver(Observer& observer) = 0;
	virtual void removeObserver(Observer& observer) = 0;
	
	virtual void realise() = 0;
	virtual void unrealise() = 0;
};
typedef boost::shared_ptr<IMapResource> IMapResourcePtr;

const std::string MODULE_MAPRESOURCEMANAGER("MapResourceManager");

class IMapResourceManager :
	public RegisterableModule
{
public:
	/**
	 * Capture a named model resource, and return a pointer to it.
	 */
	virtual IMapResourcePtr capture(const std::string& path) = 0;
};

inline IMapResourceManager& GlobalMapResourceManager() {
	// Cache the reference locally
	static IMapResourceManager& _mapResourceManager(
		*boost::static_pointer_cast<IMapResourceManager>(
			module::GlobalModuleRegistry().getModule(MODULE_MAPRESOURCEMANAGER)
		)
	);
	return _mapResourceManager;
}

#endif /* _IMAPRESOURCE_H__ */
