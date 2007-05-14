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

///\file
///\brief Represents any entity which has a fixed size specified in its entity-definition and displays a model (e.g. ammo_bfg).
///
/// This entity displays the model specified in its entity-definition.
/// The "origin" and "angle" keys directly control the entity's local-to-parent transform.
/// The "rotation" key directly controls the entity's local-to-parent transform for Doom3 only.

#include "eclassmodel.h"

#include "iregistry.h"
#include "cullable.h"
#include "renderable.h"
#include "editable.h"

#include "selectionlib.h"
#include "instancelib.h"
#include "transformlib.h"
#include "traverselib.h"
#include "entitylib.h"
#include "render.h"
#include "pivot.h"

#include "targetable.h"
#include "origin.h"
#include "angle.h"
#include "rotation.h"
#include "model.h"
#include "namedentity.h"
#include "keyobservers.h"
#include "namekeys.h"
#include "modelskinkey.h"

#include "entity.h"

class EclassModel :
  public Snappable
{
  MatrixTransform m_transform;
  Doom3Entity m_entity;
  KeyObserverMap m_keyObservers;

  OriginKey m_originKey;
  Vector3 m_origin;
  AngleKey m_angleKey;
  float m_angle;
  RotationKey m_rotationKey;
  Float9 m_rotation;
  SingletonModel m_model;

  NamedEntity m_named;
  NameKeys m_nameKeys;
  RenderablePivot m_renderOrigin;
  RenderableNamedEntity m_renderName;
  ModelSkinKey m_skin;

  Callback m_transformChanged;
  Callback m_evaluateTransform;

  void construct()
  {
    default_rotation(m_rotation);

    m_keyObservers.insert("name", NamedEntity::IdentifierChangedCaller(m_named));
    m_keyObservers.insert("angle", RotationKey::AngleChangedCaller(m_rotationKey));
    m_keyObservers.insert("rotation", RotationKey::RotationChangedCaller(m_rotationKey));
    m_keyObservers.insert("origin", OriginKey::OriginChangedCaller(m_originKey));
  }

  void updateTransform()
  {
    m_transform.localToParent() = g_matrix4_identity;
    matrix4_translate_by_vec3(m_transform.localToParent(), m_origin);

    matrix4_multiply_by_matrix4(m_transform.localToParent(), rotation_toMatrix(m_rotation));
    m_transformChanged();
  }
  typedef MemberCaller<EclassModel, &EclassModel::updateTransform> UpdateTransformCaller;

  void originChanged()
  {
    m_origin = m_originKey.m_origin;
    updateTransform();
  }
  typedef MemberCaller<EclassModel, &EclassModel::originChanged> OriginChangedCaller;
  void angleChanged()
  {
    m_angle = m_angleKey.m_angle;
    updateTransform();
  }
  typedef MemberCaller<EclassModel, &EclassModel::angleChanged> AngleChangedCaller;
  void rotationChanged()
  {
    rotation_assign(m_rotation, m_rotationKey.m_rotation);
    updateTransform();
  }
  typedef MemberCaller<EclassModel, &EclassModel::rotationChanged> RotationChangedCaller;

  void skinChanged()
  {
    scene::Node* node = m_model.getNode();
    if(node != 0)
    {
      Node_modelSkinChanged(*node);
    }
  }
  typedef MemberCaller<EclassModel, &EclassModel::skinChanged> SkinChangedCaller;

public:

  EclassModel(IEntityClassPtr eclass, scene::Node& node, const Callback& transformChanged, const Callback& evaluateTransform) :
    m_entity(eclass),
    m_originKey(OriginChangedCaller(*this)),
    m_origin(ORIGINKEY_IDENTITY),
    m_angleKey(AngleChangedCaller(*this)),
    m_angle(ANGLEKEY_IDENTITY),
    m_rotationKey(RotationChangedCaller(*this)),
    m_named(m_entity),
    m_nameKeys(m_entity),
    m_renderOrigin(m_origin),
    m_renderName(m_named, g_vector3_identity),
    m_skin(SkinChangedCaller(*this)),
    m_transformChanged(transformChanged),
    m_evaluateTransform(evaluateTransform)
  {
    construct();
  }
  EclassModel(const EclassModel& other, scene::Node& node, const Callback& transformChanged, const Callback& evaluateTransform) :
    m_entity(other.m_entity),
    m_originKey(OriginChangedCaller(*this)),
    m_origin(ORIGINKEY_IDENTITY),
    m_angleKey(AngleChangedCaller(*this)),
    m_angle(ANGLEKEY_IDENTITY),
    m_rotationKey(RotationChangedCaller(*this)),
    m_named(m_entity),
    m_nameKeys(m_entity),
    m_renderOrigin(m_origin),
    m_renderName(m_named, g_vector3_identity),
    m_skin(SkinChangedCaller(*this)),
    m_transformChanged(transformChanged),
    m_evaluateTransform(evaluateTransform)
  {
    construct();
  }

  InstanceCounter m_instanceCounter;
  void instanceAttach(const scene::Path& path)
  {
    if(++m_instanceCounter.m_count == 1)
    {
      m_entity.instanceAttach(path_find_mapfile(path.begin(), path.end()));
      m_entity.attach(m_keyObservers);
      m_model.modelChanged(m_entity.getEntityClass()->getModelPath().c_str());
      m_skin.skinChanged(m_entity.getEntityClass()->getSkin().c_str());
    }
  }
  void instanceDetach(const scene::Path& path)
  {
    if(--m_instanceCounter.m_count == 0)
    {
      m_skin.skinChanged("");
      m_model.modelChanged("");
      m_entity.detach(m_keyObservers);
      m_entity.instanceDetach(path_find_mapfile(path.begin(), path.end()));
    }
  }

  Doom3Entity& getEntity()
  {
    return m_entity;
  }
  const Doom3Entity& getEntity() const
  {
    return m_entity;
  }

  scene::Traversable& getTraversable()
  {
    return m_model.getTraversable();
  }
  const scene::Traversable& getTraversable() const
  {
    return m_model.getTraversable();
  }
  Namespaced& getNamespaced()
  {
    return m_nameKeys;
  }
  NamedEntity& getNameable()
  {
    return m_named;
  }
  const NamedEntity& getNameable() const
  {
    return m_named;
  }
  TransformNode& getTransformNode()
  {
    return m_transform;
  }
  const TransformNode& getTransformNode() const
  {
    return m_transform;
  } 
  ModelSkin& getModelSkin()
  {
    return m_skin.get();
  }
  const ModelSkin& getModelSkin() const
  {
    return m_skin.get();
  }

  void attach(scene::Traversable::Observer* observer)
  {
    m_model.attach(observer);
  }
  void detach(scene::Traversable::Observer* observer)
  {
    m_model.detach(observer);
  }

  void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
  {
    if(selected)
    {
      m_renderOrigin.render(renderer, volume, localToWorld);
    }

    renderer.SetState(m_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
  }
  void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const
  {
    renderSolid(renderer, volume, localToWorld, selected);
    if(GlobalRegistry().get("user/ui/xyview/showEntityNames") == "1")
    {
      renderer.addRenderable(m_renderName, localToWorld);
    }
  }

  void translate(const Vector3& translation)
  {
    m_origin = origin_translated(m_origin, translation);
  }
  void rotate(const Quaternion& rotation)
  {
    rotation_rotate(m_rotation, rotation);
  }
  void snapto(float snap)
  {
    m_originKey.m_origin = origin_snapped(m_originKey.m_origin, snap);
    m_originKey.write(&m_entity);
  }
  void revertTransform()
  {
    m_origin = m_originKey.m_origin;
    rotation_assign(m_rotation, m_rotationKey.m_rotation);
  }
  void freezeTransform()
  {
    m_originKey.m_origin = m_origin;
    m_originKey.write(&m_entity);
    rotation_assign(m_rotationKey.m_rotation, m_rotation);
    m_rotationKey.write(&m_entity);
  }
  void transformChanged()
  {
    revertTransform();
    m_evaluateTransform();
    updateTransform();
  }
  typedef MemberCaller<EclassModel, &EclassModel::transformChanged> TransformChangedCaller;
};

class EclassModelInstance : 
	public TargetableInstance, 
	public TransformModifier, 
	public Renderable
{
  EclassModel& m_contained;
public:
  STRING_CONSTANT(Name, "EclassModelInstance");

  EclassModelInstance(const scene::Path& path, scene::Instance* parent, EclassModel& contained) :
    TargetableInstance(path, parent, contained.getEntity(), *this),
    TransformModifier(EclassModel::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
    m_contained(contained)
  {
    m_contained.instanceAttach(Instance::path());

    StaticRenderableConnectionLines::instance().attach(*this);
  }
  ~EclassModelInstance()
  {
    StaticRenderableConnectionLines::instance().detach(*this);

    m_contained.instanceDetach(Instance::path());
  }
  void renderSolid(Renderer& renderer, const VolumeTest& volume) const
  {
    m_contained.renderSolid(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());
  }
  void renderWireframe(Renderer& renderer, const VolumeTest& volume) const
  {
    m_contained.renderWireframe(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());
  }

  void evaluateTransform()
  {
    if(getType() == TRANSFORM_PRIMITIVE)
    {
      m_contained.translate(getTranslation());
      m_contained.rotate(getRotation());
    }
  }
  void applyTransform()
  {
    m_contained.revertTransform();
    evaluateTransform();
    m_contained.freezeTransform();
  }
  typedef MemberCaller<EclassModelInstance, &EclassModelInstance::applyTransform> ApplyTransformCaller;
};

class EclassModelNode :
  public scene::Node,
  public scene::Instantiable,
  public scene::Cloneable,
  public scene::Traversable::Observer,
  public Nameable,
  public Snappable,
  public TransformNode,
  public scene::Traversable,
  public EntityNode,
  public Namespaced,
  public ModelSkin
{
  InstanceSet m_instances;
  EclassModel m_contained;

  void construct()
  {
    m_contained.attach(this);
  }
  void destroy()
  {
    m_contained.detach(this);
  }

public:
	// scene::Traversable Implementation
	virtual void insert(Node& node) {
		m_contained.getTraversable().insert(node);
	}
    virtual void erase(Node& node) {
    	m_contained.getTraversable().erase(node);	
    }
    virtual void traverse(const Walker& walker) {
    	m_contained.getTraversable().traverse(walker);
    }
    virtual bool empty() const {
    	return m_contained.getTraversable().empty();
    }
  
	// Snappable implementation
	virtual void snapto(float snap) {
		m_contained.snapto(snap);
	}

	// TransformNode implementation
	virtual const Matrix4& localToParent() const {
		return m_contained.getTransformNode().localToParent();
	}

	// EntityNode implementation
	virtual Entity& getEntity() {
		return m_contained.getEntity();
	}

	// Namespaced implementation
	virtual void setNamespace(Namespace& space) {
		m_contained.getNamespaced().setNamespace(space);
	}

	virtual void attach(ModuleObserver& observer) {
		m_contained.getModelSkin().attach(observer);
	}
	
	virtual void detach(ModuleObserver& observer) {
		m_contained.getModelSkin().detach(observer);
	}
	
	virtual std::string getRemap(const std::string& name) const {
		return m_contained.getModelSkin().getRemap(name);
	}

  EclassModelNode(IEntityClassPtr eclass) :
    m_contained(eclass, *this, InstanceSet::TransformChangedCaller(m_instances), InstanceSetEvaluateTransform<EclassModelInstance>::Caller(m_instances))
  {
    construct();
  }
  
  EclassModelNode(const EclassModelNode& other) :
    scene::Node(other),
    scene::Instantiable(other),
    scene::Cloneable(other),
    scene::Traversable::Observer(other),
    Nameable(other),
    Snappable(other),
    TransformNode(other),
    scene::Traversable(other),
    EntityNode(other),
    Namespaced(other),
    ModelSkin(other),
    m_contained(other.m_contained, *this, InstanceSet::TransformChangedCaller(m_instances), InstanceSetEvaluateTransform<EclassModelInstance>::Caller(m_instances))
  {
    construct();
  }
  ~EclassModelNode()
  {
    destroy();
  }

  scene::Node& node()
  {
    return *this;
  }

	// scene::Traversable::Observer implementation
  void insertChild(scene::Node& child)
  {
    m_instances.insertChild(child);
  }
  void eraseChild(scene::Node& child)
  {
    m_instances.eraseChild(child);
  }

  scene::Node& clone() const
  {
    return (new EclassModelNode(*this))->node();
  }

  scene::Instance* create(const scene::Path& path, scene::Instance* parent)
  {
    return new EclassModelInstance(path, parent, m_contained);
  }
  void forEachInstance(const scene::Instantiable::Visitor& visitor)
  {
    m_instances.forEachInstance(visitor);
  }
  void insert(scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance)
  {
    m_instances.insert(observer, path, instance);
  }
  scene::Instance* erase(scene::Instantiable::Observer* observer, const scene::Path& path)
  {
    return m_instances.erase(observer, path);
  }
  
	// Nameable implementation
	virtual std::string name() const {
		return m_contained.getNameable().name();
	}
	virtual void attach(const NameCallback& callback) {
		m_contained.getNameable().attach(callback);
	}
	virtual void detach(const NameCallback& callback) {
		m_contained.getNameable().detach(callback);
	}
};

scene::Node& New_EclassModel(IEntityClassPtr eclass)
{
  return (new EclassModelNode(eclass))->node();
}

