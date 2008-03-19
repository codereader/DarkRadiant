#ifndef FOREACHVISIBLE_H_
#define FOREACHVISIBLE_H_

/**
 * Scenegraph walker class which applies the given walker object to objects
 * in the scene graph depending on their intersection with the provided 
 * VolumeTest object. The walker is called on objects whose intersection test
 * returns a positive result, while objects that do not intersect the volume
 * are ignored.
 * 
 * This walker also discards entities whose entityclass is marked as invisible
 * by the filter system.
 */
template<typename Walker_>
class ForEachVisible 
: public scene::Graph::Walker
{
	// The VolumeTest object that all instances must be tested against
	const VolumeTest& m_volume;
	
	// Contained walker that will be called for each visible instance
	const Walker_& m_walker;
	
	// Stack of parent visibility values
	mutable std::vector<VolumeIntersectionValue> _visStack;

public:

	// Constructor
	ForEachVisible(const VolumeTest& volume, const Walker_& walker)
	: m_volume(volume), m_walker(walker)
	{
		_visStack.push_back(c_volumePartial);
	}
  
	// Pre-descent walker function
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		
		VolumeIntersectionValue visible = (node->visible()) 
										   ? _visStack.back() 
										   : c_volumeOutside;

		// Test for partial visibility
	    if (visible == c_volumePartial) {
			visible = m_volume.TestAABB(node->worldAABB());
	    }

		_visStack.push_back(visible);

		// Abort descent for invisible instances, otherwise invoke the contained
		// walker
		if (visible == c_volumeOutside) {
			return false;
		}
		else {
			return m_walker.pre(path, node, visible);
		}
	}
	
	// Post descent function
	void post(const scene::Path& path, const scene::INodePtr& node) const {
		// If instance was visible, call the contained walker's post-descent
		if(_visStack.back() != c_volumeOutside) {
			m_walker.post(path, node, _visStack.back());
		}

    	_visStack.pop_back();
	}
};

#endif /*FOREACHVISIBLE_H_*/
