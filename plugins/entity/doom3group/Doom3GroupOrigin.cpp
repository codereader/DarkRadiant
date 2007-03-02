#include "Doom3GroupOrigin.h"

namespace entity {

Doom3GroupOrigin::Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin) : 
	m_set(set), 
	m_origin(origin), 
	m_enabled(false) 
{}

void Doom3GroupOrigin::enable() {
	m_enabled = true;
}

void Doom3GroupOrigin::disable() {
	m_enabled = false;
}

void Doom3GroupOrigin::insert(scene::Node& node) {
	if (m_enabled) {
		// Translate the newly added child to the entity origin
		// as its coordinates are measured relatively to the entity origin
		translateNode(node, m_origin);
	}
}

void Doom3GroupOrigin::erase(scene::Node& node) {
	if (m_enabled) {
		// Translate the child back to the 0,0,0 world origin
		translateNode(node, -m_origin);
	}
}

} // namespace entity
