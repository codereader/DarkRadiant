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
		// Substract the origin from the newly inserted brush
		//translateDoom3Brush(node, -m_origin);
	}
}

void Doom3GroupOrigin::addOriginToChildren() {
	if (m_enabled) {
		m_set.traverse(Doom3BrushTranslator(m_origin));
	}
}

void Doom3GroupOrigin::removeOriginFromChildren() {
	if (m_enabled) {
		m_set.traverse(Doom3BrushTranslator(-m_origin));
	}
}

void Doom3GroupOrigin::erase(scene::Node& node) {
	if (m_enabled) {
		// Add the origin to the newly inserted brush
		//translateDoom3Brush(node, m_origin);6
	}
}

} // namespace entity
