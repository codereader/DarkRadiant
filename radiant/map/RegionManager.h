#ifndef MAP_REGIONMANAGER_
#define MAP_REGIONMANAGER_

#include "math/aabb.h"
#include "math/Vector2.h"
#include "iscenegraph.h"

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
	
	/** greebo: Returns the current regioning state (true = active)
	 */
	bool isEnabled() const;
	
	/** greebo: Retrieves a reference to the internally stored AABB.
	 */
	const AABB& getRegion() const;
	
	/** greebo: Stores the corners coordinates of the currently active
	 * 			region into the given <regionMin>, <regionMax> vectors.
	 * 
	 * Note: If region is inactive, the maximum possible bounds are returned.
	 */
	void getMinMax(Vector3& regionMin, Vector3& regionMax) const;
	
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
	
	/** greebo: The traversal function that is used to save the map to a file.
	 * 			This ensures that only regioned items are saved.
	 * 
	 * Note: the map saver passes its own walker to this function and leaves it up to it
	 * 		 whether the walker.pre() and walker.post() methods are invoked. This allows
	 * 		 filtering of the non-regioned nodes.
	 */
	static void traverseRegion(scene::Node& root, const scene::Traversable::Walker& walker);
	
	/** greebo: Saves the current selection as Region to the queried file.
	 */
	static void saveRegion();
	
	/** greebo: Disables regioning and resets the bounds.
	 */
	static void disableRegion();
	
	/** greebo: Sets the region according to the XY bounds of the current orthoview 
	 */
	static void setRegionXY();
	
	/** greebo: Sets the region to the bounds of the currently drawn brush,
	 * 			similar to the partial tall selection method.
	 * 			A single brush has to be selected (an errormsg is displayed otherwise).
	 * 
	 * Note: The brush is deleted after "use".
	 */
	static void setRegionFromBrush();
	
	/** greebo: Retrieves the AABB from the current selection and 
	 * 			takes it as new region bounds. The selection is NOT deleted.
	 * 			Not available in component selection mode.
	 * 			The selected items are de-selected after "use".
	 */
	static void setRegionFromSelection();
	
	/** greebo: Adds the region commands to the EventManager.
	 */
	static void initialiseCommands();
};

} // namespace map

map::RegionManager& GlobalRegion();

#endif /*MAP_REGIONMANAGER_*/
