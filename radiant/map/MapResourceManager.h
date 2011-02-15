#pragma once

#include "imapresource.h"
#include "map/MapResource.h"

namespace map
{

class MapResourceManager :
	public IMapResourceManager
{
public:
	// Capture a named resource.
	IMapResourcePtr capture(const std::string& path);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

}
