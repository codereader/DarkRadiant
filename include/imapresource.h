#pragma once

#include "inode.h"
#include "imodule.h"
#include "itextstream.h"
#include "imap.h"

#include <sigc++/signal.h>

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

	// Exception type thrown by the the MapResource implementation
	struct OperationException :
		public std::runtime_error 
	{
		OperationException(const std::string& msg) :
			runtime_error(msg)
		{
			rError() << "MapResource operation failed: " << msg << std::endl;
		}
	};

	/**
	* Save this resource
	*
	* It's possible to pass a mapformat to be used for saving. If the map
	* format argument is omitted, the format corresponding to the current
	* game type is used.
	*
	* Throws an OperationException if anything prevented the save from
	* completion (user cancellation, I/O errors)
	*/
	virtual void save(const map::MapFormatPtr& mapFormat = map::MapFormatPtr()) = 0;

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

	// Signal emitted when a MapExport is starting / is finished
	typedef sigc::signal<void, const scene::IMapRootNodePtr&> ExportEvent;

	// Event sent out right before a scene is sent to the exporter
	virtual ExportEvent& signal_onResourceExporting() = 0;

	// Event sent out right after a scene is sent to the exporter
	virtual ExportEvent& signal_onResourceExported() = 0;
};

inline IMapResourceManager& GlobalMapResourceManager()
{
	// Cache the reference locally
	static IMapResourceManager& _mapResourceManager(
		*std::static_pointer_cast<IMapResourceManager>(
			module::GlobalModuleRegistry().getModule(MODULE_MAPRESOURCEMANAGER)
		)
	);
	return _mapResourceManager;
}
