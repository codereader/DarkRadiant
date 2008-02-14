/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_TARGETABLE_H)
#define INCLUDED_TARGETABLE_H

#include <set>
#include <map>

#include "cullable.h"
#include "renderable.h"

#include "math/line.h"
#include "render.h"
#include "generic/callback.h"
#include "selectionlib.h"
#include "entitylib.h"
#include "stringio.h"
#include "Doom3Entity.h"

#include "target/Target.h"
#include "target/TargetManager.h"
#include "target/TargetKey.h"
#include "target/TargetKeyCollection.h"

class Targetable
{
public:
	virtual const Vector3& getWorldPosition() const = 0;
};

class TargetLinesPushBack :
	public entity::TargetKeyCollection::Visitor
{
	RenderablePointVector& m_targetLines;
	const Vector3& m_worldPosition;
	const VolumeTest& m_volume;
public:
	TargetLinesPushBack(RenderablePointVector& targetLines, 
						const Vector3& worldPosition, 
						const VolumeTest& volume) :
		m_targetLines(targetLines), 
		m_worldPosition(worldPosition), 
		m_volume(volume)
	{}

	virtual void visit(const entity::TargetPtr& target) {
		if (target->isEmpty()) {
			return;
		}

		Vector3 worldPosition = target->getPosition();
		if (m_volume.TestLine(segment_for_startend(m_worldPosition, worldPosition))) {
			m_targetLines.push_back(PointVertex(reinterpret_cast<const Vertex3f&>(m_worldPosition)));
			m_targetLines.push_back(PointVertex(reinterpret_cast<const Vertex3f&>(worldPosition)));
		}
	}
};

class RenderableTargetLines :
	public RenderablePointVector
{
	const entity::TargetKeyCollection& m_targetKeys;

public:
	RenderableTargetLines(const entity::TargetKeyCollection& targetKeys) : 
		RenderablePointVector(GL_LINES),
		m_targetKeys(targetKeys)
	{}

	void render(Renderer& renderer, const VolumeTest& volume, const Vector3& world_position) {
		if (!m_targetKeys.empty()) {
			// Clear the vector
			clear();

			// Populate the RenderablePointVector
			TargetLinesPushBack populator(*this, world_position, volume);
			m_targetKeys.forEachTarget(populator);

			if (!empty()) {
				renderer.addRenderable(*this, g_matrix4_identity);
			}
		}
	}
};

/**
 * greebo: Each targetable entity (D3Group, Speaker, Lights, etc.) derives from 
 *         this class. This applies for the entity Instances only.
 *
 * This extends the SelectableInstance interface by the Targetable interface.
 */
class TargetableInstance :
	public SelectableInstance,
	public Targetable,
	public Entity::Observer
{
	mutable Vertex3f m_position;
	entity::Doom3Entity& m_entity;
	entity::TargetKeyCollection m_targeting;
	mutable RenderableTargetLines m_renderable;

	// The current name of this entity (used for comparison in "targetNameChanged")
	std::string _targetName;

public:
	TargetableInstance(
		const scene::Path& path,
		scene::Instance* parent,
		entity::Doom3Entity& entity
	) :
		SelectableInstance(path, parent),
		m_entity(entity),
		//m_targeted(*this),
		m_renderable(m_targeting)
	{
		m_entity.attach(*this);
		m_entity.attach(m_targeting);
	}

	~TargetableInstance() {
		m_entity.detach(m_targeting);
		m_entity.detach(*this);
	}

	void setTargetsChanged(const Callback& targetsChanged) {
		m_targeting.setTargetsChanged(targetsChanged);
	}

	void targetsChanged() {
		m_targeting.targetsChanged();
	}

	// Gets called as soon as the "name" keyvalue changes
	void targetnameChanged(const std::string& name) {
		// Check if we were registered before
		if (!_targetName.empty()) {
			// Old name is not empty
			// Tell the Manager to disassociate us from the target
			entity::TargetManager::Instance().clearTarget(_targetName);
		}
		
		// Store the new name, in any case
		_targetName = name;

		if (_targetName.empty()) {
			// New name is empty, do not associate
			return;
		}

		// Tell the TargetManager to associate the name with this Instance here
		entity::TargetManager::Instance().associateTarget(_targetName, this);
	}
	typedef MemberCaller1<TargetableInstance, const std::string&, &TargetableInstance::targetnameChanged> TargetnameChangedCaller;

	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value) {
		if (key == "name") {
			// Subscribe to this keyvalue to get notified about "name" changes
			value.attach(TargetnameChangedCaller(*this));
		}
	}
	
	// Entity::Observer implementation, gets called on key erase
	void onKeyErase(const std::string& key, EntityKeyValue& value) {
		if (key == "name") {
			// Unsubscribe from this keyvalue
			value.detach(TargetnameChangedCaller(*this));
		}
	}

	// Targetable implementation
	const Vector3& getWorldPosition() const {
		const AABB& bounds = Instance::worldAABB();
		if (bounds.isValid()) {
			return bounds.getOrigin();
		}
		return localToWorld().t().getVector3();
	}
	
	void render(Renderer& renderer, const VolumeTest& volume) const {
		renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
		renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eFullMaterials);
		m_renderable.render(renderer, volume, getWorldPosition());
	}
};

/**
 * greebo: This is a "container" for all TargetableInstances. These register
 *         themselves at construction time and will get invoked during
 *         the frontend render pass.
 *
 * This object is also a Renderable which is always attached to the GlobalShaderCache()
 * during the entire module lifetime.
 */
class RenderableConnectionLines : 
	public Renderable
{
	typedef std::set<TargetableInstance*> TargetableInstances;
	TargetableInstances m_instances;
public:
	// Add a TargetableInstance to this set
	void attach(TargetableInstance& instance) {
		ASSERT_MESSAGE(m_instances.find(&instance) == m_instances.end(), "cannot attach instance");
		m_instances.insert(&instance);
	}

	void detach(TargetableInstance& instance) {
		ASSERT_MESSAGE(m_instances.find(&instance) != m_instances.end(), "cannot detach instance");
		m_instances.erase(&instance);
	}

	void renderSolid(Renderer& renderer, const VolumeTest& volume) const {
		for (TargetableInstances::const_iterator i = m_instances.begin(); i != m_instances.end(); ++i) {
			if((*i)->path().top()->visible())
			{
				(*i)->render(renderer, volume);
			}
		}
	}

	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
		renderSolid(renderer, volume);
	}
};

typedef Static<RenderableConnectionLines> StaticRenderableConnectionLines;

#endif
