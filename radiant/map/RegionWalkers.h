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

/** greebo: This class is used indirectly by the map saving walker to save the region.
 * 
 * The map saving walker calls a function RegionManager::traverseRegion() which
 * traverses the given node with this walker. 
 * 
 * This ExcludeNonRegionedWalker again invokes the map saving walker, if the
 * visited items are regioned only, of course.
 */
class ExcludeNonRegionedWalker : 
	public scene::Traversable::Walker
{
	const scene::Traversable::Walker& _walker;
	mutable bool _skip;

public:
	ExcludeNonRegionedWalker(const scene::Traversable::Walker& walker) : 
		_walker(walker), 
		_skip(false) 
	{}
	
	bool pre(scene::Node& node) const {
		// Don't save excluded nodes or the Scenegraph root
		if (node.excluded() || node.isRoot()) {
			_skip = true;
			return false;
		}
		else {
			// Item passed the check, call the given walker's pre() method.
			_walker.pre(node);
		}
		return true;
	}

	void post(scene::Node& node) const {
		if (_skip) {
			// The node failed to pass the check in pre()
			_skip = false;
		}
		else {
			// The node passed the check in pre(), we have to call post() as well. 
			_walker.post(node);
		}
	}
};

} // namespace map

#endif /*REGIONWALKERS_H_*/
