#ifndef FOREACHVISIBLE_H_
#define FOREACHVISIBLE_H_

#include "ientity.h"
#include "ifilter.h"
#include "ieclass.h"

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
	const VolumeTest& m_volume;
	const Walker_& m_walker;
	mutable std::vector<VolumeIntersectionValue> m_state;

public:

	// Constructor
	ForEachVisible(const VolumeTest& volume, const Walker_& walker)
	: m_volume(volume), m_walker(walker)
	{
		m_state.push_back(c_volumePartial);
	}
  
	// Pre-descent walker function
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		
		VolumeIntersectionValue visible = (path.top().get().visible()) 
										   ? m_state.back() 
										   : c_volumeOutside;

		// Examine the entity class for its filter status. If it is filtered, 
		// use the c_volumeOutside state to ensure it is not rendered.
		Entity* entity = Node_getEntity(path.top());
		if (entity) {
			const IEntityClass& eclass = entity->getEntityClass();
			if (!GlobalFilterSystem().isVisible("entityclass", 
												eclass.getName())) 
			{
				visible = c_volumeOutside;
			}
		}
	
		// Test for partial visibility
	    if(visible == c_volumePartial) {
			visible = m_volume.TestAABB(instance.worldAABB());
	    }

		m_state.push_back(visible);

		// Abort descent for invisible instances, otherwise invoke the contained
		// walker
		if (visible == c_volumeOutside) {
			return false;
		}
		else {
			return m_walker.pre(path, instance, m_state.back());
		}
	}
	
	// Post descent function
	void post(const scene::Path& path, scene::Instance& instance) const {
		
		// If instance was visible, call the contained walker's post-descent
		if(m_state.back() != c_volumeOutside) {
			m_walker.post(path, instance, m_state.back());
		}

    	m_state.pop_back();
	}
};

#endif /*FOREACHVISIBLE_H_*/
