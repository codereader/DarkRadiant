#ifndef _ENTITY_TARGETABLEINSTANCE_H_
#define _ENTITY_TARGETABLEINSTANCE_H_

#include "selectionlib.h"
#include "entitylib.h"

#include "../Doom3Entity.h"

#include "TargetKeyCollection.h"
#include "RenderableTargetLines.h"

namespace entity {

/**
 * greebo: Each targetable entity (D3Group, Speaker, Lights, etc.) derives from 
 *         this class. This applies for the entity Instances only.
 *
 * This registers itself with the contained Doom3Entity and observes its keys.
 * As soon as "name" keys are encountered, the TargetManager is notified about
 * the change, so that the name can be associated with a Target object.
 */
class TargetableInstance :
	public SelectableInstance,
	public Entity::Observer
{
	mutable Vertex3f m_position;
	Doom3Entity& _entity;
	TargetKeyCollection _targetKeys;
	mutable RenderableTargetLines _renderableLines;

	// The current name of this entity (used for comparison in "targetNameChanged")
	std::string _targetName;

public:
	TargetableInstance(const scene::Path& path, scene::Instance* parent, Doom3Entity& entity);
	virtual ~TargetableInstance();

	void setTargetsChanged(const Callback& targetsChanged);
	void targetsChanged();

	// Gets called as soon as the "name" keyvalue changes
	void targetnameChanged(const std::string& name);
	typedef MemberCaller1<TargetableInstance, const std::string&, &TargetableInstance::targetnameChanged> TargetnameChangedCaller;

	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value);
	
	// Entity::Observer implementation, gets called on key erase
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	void render(Renderer& renderer, const VolumeTest& volume) const;

private:
	// Helper method to retrieve the current position
	const Vector3& getWorldPosition() const;
};

} // namespace entity

#endif /* _ENTITY_TARGETABLEINSTANCE_H_ */
