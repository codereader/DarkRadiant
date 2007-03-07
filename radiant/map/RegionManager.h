#ifndef MAP_REGIONMANAGER_
#define MAP_REGIONMANAGER_

#include "math/aabb.h"
#include "math/Vector2.h"

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
	
	/** greebo: Applies the currently active AABB to the scenegraph.
	 * This excludes all nodes out of the current region bounds. 
	 */
	void enable();
	
	// Disables regioning and resets the bounds to _worldMax
	void disable();
	
	/** greebo: Sets the region bounds according to the given <aabb>
	 * 
	 * Note: passing an invalid AABB disables the regioning. 
	 */
	void setRegion(const AABB& aabb);
	
	/** greebo: sets the region bounds from the given rectangle in the XY plane.
	 * 
	 * The z-component is being stretched from worldMin to worldMax, so that the
	 * region contains the entire height of the map.
	 */
	void setRegionFromXY(Vector2 topLeft, Vector2 lowerRight);
	
	// Static command targets for use in EventManager
	
	/** greebo: Saves the current selection as Region to the queried file.
	 */
	static void saveRegion();
	
	/** greebo: Disables regioning and resets the bounds.
	 */
	static void disableRegion();
	
	/** greebo: Sets the region according to the XY bounds of the current orthoview 
	 */
	static void setRegionXY();
};

} // namespace map

map::RegionManager& GlobalRegion();

#endif /*MAP_REGIONMANAGER_*/
