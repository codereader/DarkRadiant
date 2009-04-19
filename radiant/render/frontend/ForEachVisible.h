#ifndef FOREACHVISIBLE_H_
#define FOREACHVISIBLE_H_

#include <inode.h>

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
class ForEachVisible : 
	public scene::NodeVisitor
{
	// The VolumeTest object that all instances must be tested against
	const VolumeTest& m_volume;
	
	// Contained walker that will be called for each visible instance
	const Walker_& m_walker;
	
	// Stack of parent visibility values
	std::vector<VolumeIntersectionValue> _visStack;

public:

	// Constructor
	ForEachVisible(const VolumeTest& volume, const Walker_& walker) : 
		m_volume(volume), m_walker(walker)
	{
		_visStack.push_back(VOLUME_PARTIAL);
	}
  
	// Pre-descent walker function
	bool pre(const scene::INodePtr& node) {
		
		VolumeIntersectionValue visible = (node->visible()) 
										   ? _visStack.back() 
										   : VOLUME_OUTSIDE;

		// Test for partial visibility
	    if (visible == VOLUME_PARTIAL) {
			visible = m_volume.TestAABB(node->worldAABB());
	    }

		_visStack.push_back(visible);

		// Abort descent for invisible instances, otherwise invoke the contained
		// walker
		if (visible == VOLUME_OUTSIDE) {
			return false;
		}
		else {
			return m_walker.pre(node, visible);
		}
	}
	
	// Post descent function
	void post(const scene::INodePtr& node) {
		// If instance was visible, call the contained walker's post-descent
		if(_visStack.back() != VOLUME_OUTSIDE) {
			m_walker.post(node, _visStack.back());
		}

    	_visStack.pop_back();
	}
};

#endif /*FOREACHVISIBLE_H_*/
