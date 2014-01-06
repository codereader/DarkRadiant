#pragma once

#include "selectionlib.h"
#include "scene/Node.h"
#include "entitylib.h"

#include "../Doom3Entity.h"

#include "TargetKeyCollection.h"
#include "RenderableTargetLines.h"

namespace entity {

/**
 * greebo: Each targetable entity (D3Group, Speaker, Lights, etc.) derives from
 *         this class.
 *
 * This registers itself with the contained Doom3Entity and observes its keys.
 * As soon as "name" keys are encountered, the TargetManager is notified about
 * the change, so that the name can be associated with a Target object.
 */
class TargetableNode :
	public Entity::Observer,
	public KeyObserver
{
	mutable Vertex3f m_position;
	Doom3Entity& _d3entity;
	TargetKeyCollection _targetKeys;
	mutable RenderableTargetLines _renderableLines;

	// The current name of this entity (used for comparison in "onKeyValueChanged")
	std::string _targetName;

	// The node we're associated with
	scene::Node& _node;

	const ShaderPtr& _wireShader;

public:
	TargetableNode(Doom3Entity& entity, scene::Node& node, const ShaderPtr& wireShader);

	// Connect this class with the Doom3Entity
	void construct();
	// Disconnect this class from the entity
	void destruct();

	// Gets called as soon as the "name" keyvalue changes
	void onKeyValueChanged(const std::string& name);

	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value);

	// Entity::Observer implementation, gets called on key erase
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	void render(RenderableCollector& collector, const VolumeTest& volume) const;

private:
	// Helper method to retrieve the current position
	const Vector3& getWorldPosition() const;
};

} // namespace entity
