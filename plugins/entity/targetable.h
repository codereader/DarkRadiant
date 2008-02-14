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

class Targetable
{
public:
	virtual const Vector3& getWorldPosition() const = 0;
};

namespace entity {

class TargetManager
{
	// The list of all named Target objects
	typedef std::map<std::string, TargetPtr> TargetList;
	TargetList _targets;

	// An empty Target
	TargetPtr _emptyTarget;

	// Private constructor
	TargetManager();
public:
	// Accessor to the singleton instance
	static TargetManager& Instance();

	/**
	 * greebo: Returns the Target with the given name.
	 *         
	 * This never returns NULL, a Target is created if it doesn't exist yet.
	 */
	TargetPtr getTarget(const std::string name);

	/**
	 * greebo: Associates the Target with the given name
	 *         to the given scene::Instance.
	 *
	 * The Target will be created if it doesn't exist yet.
	 */
	void associateTarget(const std::string& name, scene::Instance* instance);

	/**
	 * greebo: Disassociates the Target from the given name.
	 */
	void clearTarget(const std::string& name);
};

/**
 * greebo: A TargetingEntity encapsulates a "targetN" key of a given entity. 
 * It acts as Observer for this key and maintains a pointer to the named Target.
 *
 * Note: An Entity can have multiple "targetN" keys, hence it can hold multiple 
 * instances of this TargetingEntity class.
 *
 * At any rate, each TargetKey can only refer to one Target.
 */ 
class TargetKey
{
	// The target of this key
	TargetPtr _target;
public:
	// Accessor method for the contained TargetPtr
	const TargetPtr& getTarget() const;

	// Observes the given keyvalue
	void attachToKeyValue(EntityKeyValue& value);

	// Stops observing the given keyvalue
	void detachFromKeyValue(EntityKeyValue& value);

	// This gets called as soon as the "target" key in the spawnargs changes
	void targetChanged(const std::string& target);
	// Shortcut typedef
	typedef MemberCaller1<TargetKey, const std::string&, &TargetKey::targetChanged> TargetChangedCaller;
};

} // namespace entity


// Looks up a named Targetable (returns NULL if not found)
Targetable* getTargetable(const std::string& targetname);

typedef std::map<std::string, Targetable*> TargetableMap;
extern TargetableMap g_targetables;

class TargetedEntity
{
	Targetable& m_targetable;
	std::string _name;
  
	void construct() {
		if (_name.empty()) return;

		g_targetables.insert(
			TargetableMap::value_type(_name, &m_targetable)
		);
	}

	void destroy() {
		if (_name.empty()) return;

		TargetableMap::iterator i = g_targetables.find(_name);
		if (i != g_targetables.end()) {
			g_targetables.erase(i);
		}
	}

public:
	TargetedEntity(Targetable& targetable) : 
		m_targetable(targetable)
	{
		construct();
	}

	~TargetedEntity() {
		destroy();
	}

	void targetnameChanged(const std::string& name) {
		// Change the mapping of this Targetable.
		destroy();
		_name = name;
		construct();
	}
	typedef MemberCaller1<TargetedEntity, const std::string&, &TargetedEntity::targetnameChanged> TargetnameChangedCaller;
};

/**
 * greebo: A TargetingEntity encapsulates a "targetN" key of a given entity. 
 * It acts as Observer for this key and maintains a pointer to the named Targetable.
 *
 * Note: An Entity can have multiple "targetN" keys, hence it can hold multiple 
 * instances of this TargetingEntity class.
 *
 * At any rate, each TargetingEntity can only refer to one Targetable.
 */ 
class TargetingEntity
{
	// The targetable the "targetN" key is pointing to (can be NULL)
	Targetable* _targetable;

public:
	TargetingEntity() :
		_targetable(NULL)
	{}

	// This gets called as soon as the "target" key in the spawnargs changes
	void targetChanged(const std::string& target) {
		_targetable = getTargetable(target);
	}
	typedef MemberCaller1<TargetingEntity, const std::string&, &TargetingEntity::targetChanged> TargetChangedCaller;

	bool empty() const {
		return _targetable == NULL;
	}

	template<typename Functor>
	void forEachTarget(const Functor& functor) const {
		if (_targetable == NULL) {
			return;
		}

		functor(_targetable->getWorldPosition());
	}
};

class TargetKeys : 
	public Entity::Observer
{
public:
	class Visitor {
	public:
		// Gets called with each Target contained in the TargetKeys object
		virtual void visit(const entity::TargetPtr& target) = 0;
	};

private:
	// greebo: A container mapping "targetN" keys to TargetKey objects
	typedef std::map<std::string, entity::TargetKey> TargetKeyMap;
	TargetKeyMap _targetKeys;

	Callback _targetsChanged;

public:
	void setTargetsChanged(const Callback& targetsChanged);

	// Entity::Observer implementation, gets called on key insert/erase
	void onKeyInsert(const std::string& key, EntityKeyValue& value);
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	/**
	 * greebo: Walker function, calls visit() for each target
	 *         contained in this structure.
	 */
	void forEachTarget(Visitor& visitor) const {
		for (TargetKeyMap::const_iterator i = _targetKeys.begin(); i != _targetKeys.end(); i++) {
			visitor.visit(i->second.getTarget());
		}
	}

	// Returns TRUE if there are no "target" keys observed
	bool empty() const {
		return _targetKeys.empty();
	}

	//const TargetKeyMap& get() const;

	// Triggers a callback that the targets have been changed
	void targetsChanged();
private:
	bool isTargetKey(const std::string& key);
};


class TargetLinesPushBack :
	public TargetKeys::Visitor
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

/*class RenderableTargetingEntity
{
  TargetingEntity& m_targets;
  mutable RenderablePointVector m_target_lines;
public:
  static ShaderPtr m_state;

  RenderableTargetingEntity(TargetingEntity& targets)
    : m_targets(targets), m_target_lines(GL_LINES)
  {
  }
  void compile(const VolumeTest& volume, const Vector3& world_position) const
  {
    m_target_lines.clear();

	if (m_targets.empty()) {
		return;
	}

    m_target_lines.reserve(2);
	m_targets.forEachTarget(TargetLinesPushBack(m_target_lines, world_position, volume));
  }
  void render(Renderer& renderer, const VolumeTest& volume, const Vector3& world_position) const
  {
    if (!m_targets.empty())
    {
      compile(volume, world_position);
      if(!m_target_lines.empty())
      {
        renderer.addRenderable(m_target_lines, g_matrix4_identity);
      }
    }
  }
};*/

class RenderableTargetLines :
	public RenderablePointVector
{
	const TargetKeys& m_targetKeys;

public:
	//static ShaderPtr m_state;

	RenderableTargetLines(const TargetKeys& targetKeys) : 
		RenderablePointVector(GL_LINES),
		m_targetKeys(targetKeys)
	{}

	/*void compile(const VolumeTest& volume, const Vector3& world_position) const {
		m_target_lines.clear();
		m_targetKeys.forEachTargetingEntity();
	}*/

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
			
			/*compile(volume, world_position);
			if (!m_target_lines.empty()) {
				renderer.addRenderable(m_target_lines, g_matrix4_identity);
			}*/
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
	TargetKeys m_targeting;
	//TargetedEntity m_targeted;
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
