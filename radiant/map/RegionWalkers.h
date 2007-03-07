#ifndef REGIONWALKERS_H_
#define REGIONWALKERS_H_

#include "scenelib.h"

namespace map {

/** greebo: Sets the "excluded" bit in the Node according to the given boolean.
 * 
 * @exclude: set to TRUE if you want to exclude the node.
 */
inline void excludeNode(scene::Node& node, bool exclude) {
	if (exclude) {
		node.enable(scene::Node::eExcluded);
	}
	else {
		node.disable(scene::Node::eExcluded);
	}
}

/** greebo: Sets/resets the exclude bit on the visited node
 */
class ExcludeAllWalker : 
	public scene::Graph::Walker
{
	bool _exclude;
public:
	ExcludeAllWalker(bool exclude) : 
		_exclude(exclude) 
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		excludeNode(path.top(), _exclude);

		return true;
	}
};

/** greebo: Sets the exclusion status of all the objects WITHIN the bounds.
 * 
 * All the objects outside the bounds get their status set to the opposite.
 */
class ExcludeRegionedWalker : 
	public scene::Graph::Walker
{
	bool _exclude;
	// The reference to the currently active region bounds
	AABB& _regionAABB;

public:
	ExcludeRegionedWalker(bool exclude, AABB& regionAABB) : 
		_exclude(exclude),
		_regionAABB(regionAABB)
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		
		// Check whether the instance is within the region
		bool contained = aabb_intersects_aabb(
			instance.worldAABB(),
			_regionAABB
		);
		
		if (contained) {
			// The contained stuff is set according to the _exclude parameter
			excludeNode(path.top(), _exclude);
		}
		else {
			// This is an object outside the bounds, set it to !_exclude
			// as the _exclude should apply to the objects within.
			excludeNode(path.top(), !_exclude);
		}
		
		// Traverse the children as well
		return true;
	}
};


} // namespace map

#endif /*REGIONWALKERS_H_*/
