#ifndef MAP_REGIONMANAGER_
#define MAP_REGIONMANAGER_

#include "math/aabb.h"

namespace map {

class RegionManager
{
	// TRUE, if regioning is active 
	bool _active;
	
	// The largest/smalles possible world coordinates (+/- 65536)
	float _worldMin;
	float _worldMax;
	
	// The bounds of this region
	AABB _bounds;
	
public:
	RegionManager();
	
	// Disables regioning and resets the bounds to _worldMax
	void disable();
	
	// Static command targets for use in EventManager
	
	/** greebo: Saves the current selection as Region to the queried file.
	 */
	static void saveRegion();
	
	/** greebo: Disables regioning and resets the bounds.
	 */
	static void disableRegion();
};

} // namespace map

map::RegionManager& GlobalRegion();

#endif /*MAP_REGIONMANAGER_*/
