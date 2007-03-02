#ifndef DOOM3GROUPORIGIN_H_
#define DOOM3GROUPORIGIN_H_

#include "scenelib.h"

namespace entity {

class Doom3GroupOrigin : 
	public scene::Traversable::Observer
{
	scene::Traversable& m_set;
	const Vector3& m_origin;
	bool m_enabled;

public:
	Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin);

	// Enable the callbacks, set the bool to TRUE if a callback should 
	// be triggered immediately.
	void enable(bool triggerOriginChange = true);
	void disable();

	// Triggers a setDoom3GroupOrigin call on all the child brushes
	// (which basically moves them according to the origin).
	void originChanged();

	void insert(scene::Node& node);
	void erase(scene::Node& node);
};

} // namespace entity

#endif /*DOOM3GROUPORIGIN_H_*/
