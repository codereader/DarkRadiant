#ifndef REGIONWALKERS_H_
#define REGIONWALKERS_H_

#include "scenelib.h"

namespace map {

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


} // namespace map

#endif /*REGIONWALKERS_H_*/
