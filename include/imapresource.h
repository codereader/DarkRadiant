#pragma once

#include "inode.h"
#include "imodule.h"
#include "imap.h"

namespace map 
{ 
	class MapFormat; 
	typedef std::shared_ptr<MapFormat> MapFormatPtr;
}

class IMapResource
{
public:
	virtual ~IMapResource() {}

	// Renames this map resource to the new path
	virtual void rename(const std::string& fullPath) = 0;

	virtual bool load() = 0;
	virtual bool save(const map::MapFormatPtr& mapFormat = map::MapFormatPtr()) = 0;

    virtual scene::IMapRootNodePtr getNode() = 0;
    virtual void setNode(const scene::IMapRootNodePtr& node) = 0;
};
typedef std::shared_ptr<IMapResource> IMapResourcePtr;

const char* const MODULE_MAPRESOURCEMANAGER("MapResourceManager");

class IMapResourceManager :
	public RegisterableModule
{
public:
	/**
	 * Load the named map resource from VFS or from a physical path.
	 */
	virtual IMapResourcePtr loadFromPath(const std::string& path) = 0;
};

inline IMapResourceManager& GlobalMapResourceManager() {
	// Cache the reference locally
	static IMapResourceManager& _mapResourceManager(
		*std::static_pointer_cast<IMapResourceManager>(
			module::GlobalModuleRegistry().getModule(MODULE_MAPRESOURCEMANAGER)
		)
	);
	return _mapResourceManager;
}
