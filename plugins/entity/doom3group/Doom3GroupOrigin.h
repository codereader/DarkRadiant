#ifndef DOOM3GROUPORIGIN_H_
#define DOOM3GROUPORIGIN_H_

#include "scenelib.h"

namespace entity {

/** greebo: This is an Observer helper class that gets notified 
 * on incoming child nodes. These are translated with the given origin
 * on insertion which takes care that the child brushes/patches are
 * measured relatively to the Doom3Group origin key. 
 */
class Doom3GroupOrigin : 
	public scene::Traversable::Observer
{
	scene::Traversable& m_set;
	const Vector3& m_origin;
	bool m_enabled;

public:
	Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin);

	// Enable the automatic translation of child nodes
	void enable();
	void disable();
	
	/** greebo: Translates all child brushes about +/-m_origin to store
	 * their position relatively to the worldspawn/func_static origin.
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

	// scene::Traversable::Observer implementation
	void insertChild(scene::Node& node);
	void eraseChild(scene::Node& node);
};

} // namespace entity

#endif /*DOOM3GROUPORIGIN_H_*/
