#pragma once

#include "imapresource.h"
#include "map/MapResource.h"

namespace map
{

class MapResourceManager :
	public IMapResourceManager
{
public:
	IMapResourcePtr loadFromPath(const std::string& path) override;

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
};

}
