#ifndef RADIANTREFERENCECACHE_H_
#define RADIANTREFERENCECACHE_H_

#include <map>
#include "ireference.h"
#include "ifilesystem.h"
#include "map/MapResource.h"

class RadiantReferenceCache : 
	public ReferenceCache, 
	public VirtualFileSystem::Observer
{
	// Map of named MapResource objects
	typedef std::map<std::string, map::MapResourceWeakPtr> MapReferences;
	MapReferences _mapReferences;
  
	bool _realised;

public:
	RadiantReferenceCache();

	// Capture a named resource.
	ResourcePtr capture(const std::string& path);
	
	// Saves all map resources
	void saveReferences();
	
	// Returns true if all MapResources are saved.
	bool referencesSaved();	
	
	bool realised() const;
	void realise();
	void unrealise();
		
	void clear();

	// Gets called on VFS initialise
  	virtual void onFileSystemInitialise();
  	// Gets called on VFS shutdown
  	virtual void onFileSystemShutdown();
  
	// Command target: calls refresh() and resets the ModelPreview
	void refreshReferences();
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

private:
  	// Branch for capturing mapfile resources
  	ResourcePtr captureMap(const std::string& path);
};

#endif /*RADIANTREFERENCECACHE_H_*/
