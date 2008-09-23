#ifndef _IMAPRESOURCE_H__
#define _IMAPRESOURCE_H__

#include "inode.h"
#include "imodule.h"
#include "ireference.h"

const std::string MODULE_MAPRESOURCEMANAGER("MapResourceManager");

class IMapResourceManager :
	public RegisterableModule
{
public:
	/**
	 * Capture a named model resource, and return a pointer to it.
	 */
	virtual ResourcePtr capture(const std::string& path) = 0;
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
