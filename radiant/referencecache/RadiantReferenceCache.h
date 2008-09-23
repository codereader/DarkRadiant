#ifndef RADIANTREFERENCECACHE_H_
#define RADIANTREFERENCECACHE_H_

#include "ireference.h"
#include "map/MapResource.h"

class RadiantReferenceCache : 
	public ReferenceCache
{
public:
	// Capture a named resource.
	ResourcePtr capture(const std::string& path);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:
  	// Branch for capturing mapfile resources
  	ResourcePtr captureMap(const std::string& path);
};

#endif /*RADIANTREFERENCECACHE_H_*/
