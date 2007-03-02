#include "Doom3GroupOrigin.h"

namespace entity {

/** greebo: This checks for primitive nodes of types BrushDoom3/PatchDoom3 and passes 
 * the new origin to them so they can take the according actions like
 * TexDef translation in case of an active texture lock, etc.
 */
void Primitives_setDoom3GroupOrigin(scene::Node& node, const Vector3& origin) {
	// Check for BrushDoom3
	BrushDoom3* brush = Node_getBrushDoom3(node);
	if (brush != NULL) {
		brush->setDoom3GroupOrigin(origin);
	}
}

class SetDoom3GroupOriginWalker : 
	public scene::Traversable::Walker
{
	const Vector3& m_origin;
public:
	SetDoom3GroupOriginWalker(const Vector3& origin) : 
		m_origin(origin) 
	{}
	
	bool pre(scene::Node& node) const {
		Primitives_setDoom3GroupOrigin(node, m_origin);
		return true;
	}
};

Doom3GroupOrigin::Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin) : 
	m_set(set), 
	m_origin(origin), 
	m_enabled(false) 
{}

void Doom3GroupOrigin::enable(bool triggerOriginChange) {
	m_enabled = true;
	if (triggerOriginChange) {
		originChanged();
	}
}

void Doom3GroupOrigin::disable() {
	m_enabled = false;
}

void Doom3GroupOrigin::originChanged() {
	if (m_enabled) {
		//m_set.traverse(SetDoom3GroupOriginWalker(m_origin));
	}
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
