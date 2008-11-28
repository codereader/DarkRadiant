#ifndef RADIANTREFERENCECACHE_H_
#define RADIANTREFERENCECACHE_H_

#include "imapresource.h"
#include "map/MapResource.h"

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

#endif /*RADIANTREFERENCECACHE_H_*/
