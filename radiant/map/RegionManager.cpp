#include "RegionManager.h"

#include "iregistry.h"
#include "iscenegraph.h"
#include <boost/shared_ptr.hpp>

#include "RegionWalkers.h"

namespace map {

	namespace {
		typedef boost::shared_ptr<RegionManager> RegionManagerPtr;
	}

RegionManager::RegionManager() :
	_active(false)
{
	_worldMin = GlobalRegistry().getFloat("game/defaults/minWorldCoord");
	_worldMax = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
}

void RegionManager::disable() {
	_active = false;

	_bounds = AABB::createFromMinMax(Vector3(1,1,1)*_worldMin, Vector3(1,1,1)*_worldMax);
	// Create a 64 unit border from the max world coord
	_bounds.extents -= Vector3(1,1,1)*64;
	
	// Disable the exclude bit on all the scenegraph nodes
	GlobalSceneGraph().traverse(ExcludeAllWalker(false));
}

// Static members (used as command targets for EventManager)

void RegionManager::disableRegion() {
	GlobalRegion().disable();
	SceneChangeNotify();
}

void RegionManager::saveRegion() {
	
}

} // namespace map

map::RegionManager& GlobalRegion() {
	static map::RegionManagerPtr _regionManager;
	
	if (_regionManager == NULL) {
		_regionManager = map::RegionManagerPtr(new map::RegionManager);
	}
	
	return *_regionManager;
}
