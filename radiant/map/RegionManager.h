#ifndef MAP_REGIONMANAGER_
#define MAP_REGIONMANAGER_

#include <list>
#include "math/aabb.h"
#include "math/Vector2.h"
#include "iscenegraph.h"

/** greebo: The RegionManager provides methods to enable/disable
 * 			the regioning when map editing as well as functions
 * 			to set the region bounds from brushes/xyview/current selection.
 * 
 * Regioned nodes are hidden during map editing (they get their excluded bit set).
 * It's still possible to apply additional filtering to a region via show/hide,
 * these systems are independent of each other.
 * 
 * @SaveRegion: This saves the current region (all non-excluded nodes) to a
 * 				specified file and places six wall/floor/ceiling brushes to
 * 				the file together with an info_player_start entity.
 * 				The info_player_start is placed at the current camera position.  
 * 
 * The static members are used to connect the EntityManager with the 
 * according commands, as the FreeCaller<> stuff needs static functions.
 */
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
	
	// The brushes around the region boundaries 
	// (legacy array to stay compatible with the ConstructRegionBrushes() function)
	scene::INodePtr _brushes[6];
	
	// The pointer to the info_player_start entity
	scene::INodePtr _playerStart;
	
public:
	RegionManager();

	// Clears the region information (and releases the wall brushes too)
	void clear();
	
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
	 * 
	 * @autoEnable: set this to false to prevent the scenegraph from being
	 * 				traversed and its nodes being hidden (enable() is not called).
	 */
	void setRegion(const AABB& aabb, bool autoEnable = true);
	
	/** greebo: sets the region bounds from the given rectangle in the XY plane.
	 * 
	 * The z-component is being stretched from worldMin to worldMax, so that the
	 * region contains the entire height of the map.
	 */
	void setRegionFromXY(Vector2 topLeft, Vector2 lowerRight);
	
	/** greebo: Adds the bounding brushes that enclose the current region.
	 */
	void addRegionBrushes();
	
	/** greebo: Removes the bounding brushes added by addRegionBrushes().
	 */
	void removeRegionBrushes();
	
	/** greebo: The traversal function that is used to save the map to a file.
	 * 			This ensures that only regioned items are saved.
	 * 
	 * Note: the map saver passes its own walker to this function and leaves it up to it
	 * 		 whether the walker.pre() and walker.post() methods are invoked. This allows
	 * 		 filtering of the non-regioned nodes.
	 */
	static void traverseRegion(scene::INodePtr root, scene::NodeVisitor& walker);
	
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

// Use this to call the non-static member methods.
map::RegionManager& GlobalRegion();

#endif /*MAP_REGIONMANAGER_*/
