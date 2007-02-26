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
///\brief Represents any Doom3 entity which does not have a fixed size specified in its entity-definition (e.g. func_static).
///
/// This entity behaves as a group only when the "model" key is empty or is the same as the "name" key. Otherwise it behaves as a model.
/// When behaving as a group, the "origin" key is the translation to be applied to all brushes (not patches) grouped under this entity.
/// When behaving as a model, the "origin", "angle" and "rotation" keys directly control the entity's local-to-parent transform.
/// When either the "curve_Nurbs" or "curve_CatmullRomSpline" keys define a curve, the curve is rendered and can be edited.

#include "doom3group.h"

#include "iregistry.h"
#include "cullable.h"
#include "renderable.h"
#include "editable.h"
#include "modelskin.h"

#include "selectionlib.h"
#include "instancelib.h"
#include "transformlib.h"
#include "traverselib.h"
#include "entitylib.h"
#include "render.h"
#include "stream/stringstream.h"
#include "pivot.h"

#include "targetable.h"
#include "origin.h"
#include "angle.h"
#include "rotation.h"
#include "model.h"
#include "namedentity.h"
#include "keyobservers.h"
#include "namekeys.h"
#include "curve.h"
#include "modelskinkey.h"

#include "entity.h"

#include "doom3group/Doom3GroupInstance.h"

class Doom3GroupNode :
  public scene::Node::Symbiot,
  public scene::Instantiable,
  public scene::Cloneable,
  public scene::Traversable::Observer
{
  class TypeCasts
  {
    NodeTypeCastTable m_casts;
  public:
    TypeCasts()
    {
      NodeStaticCast<Doom3GroupNode, scene::Instantiable>::install(m_casts);
      NodeStaticCast<Doom3GroupNode, scene::Cloneable>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, scene::Traversable>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, Snappable>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, TransformNode>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, Entity>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, Nameable>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, Namespaced>::install(m_casts);
      NodeContainedCast<Doom3GroupNode, ModelSkin>::install(m_casts);
    }
    NodeTypeCastTable& get()
    {
      return m_casts;
    }
  };


  scene::Node m_node;
  InstanceSet m_instances;
  entity::Doom3Group m_contained;

  void construct()
  {
    m_contained.attach(this);
  }
  void destroy()
  {
    m_contained.detach(this);
  }
public:

  typedef LazyStatic<TypeCasts> StaticTypeCasts;

  scene::Traversable& get(NullType<scene::Traversable>)
  {
    return m_contained.getTraversable();
  }
  Snappable& get(NullType<Snappable>)
  {
    return m_contained;
  }
  TransformNode& get(NullType<TransformNode>)
  {
    return m_contained.getTransformNode();
  }
  Entity& get(NullType<Entity>)
  {
    return m_contained.getEntity();
  }
  Nameable& get(NullType<Nameable>)
  {
    return m_contained.getNameable();
  }
  Namespaced& get(NullType<Namespaced>)
  {
    return m_contained.getNamespaced();
  }
  ModelSkin& get(NullType<ModelSkin>)
  {
    return m_contained.getModelSkin();
  }

  Doom3GroupNode(IEntityClassPtr eclass) :
    m_node(this, this, StaticTypeCasts::instance().get()),
      m_contained(
      	eclass, 
      	m_node, 
      	InstanceSet::TransformChangedCaller(m_instances), 
      	InstanceSet::BoundsChangedCaller(m_instances), 
      	InstanceSetEvaluateTransform<entity::Doom3GroupInstance>::Caller(m_instances)
      )
  {
    construct();
  }
  Doom3GroupNode(const Doom3GroupNode& other) :
    scene::Node::Symbiot(other),
    scene::Instantiable(other),
    scene::Cloneable(other),
    scene::Traversable::Observer(other),
    m_node(this, this, StaticTypeCasts::instance().get()),
    m_contained(
    	other.m_contained, 
    	m_node, 
    	InstanceSet::TransformChangedCaller(m_instances), 
    	InstanceSet::BoundsChangedCaller(m_instances), 
    	InstanceSetEvaluateTransform<entity::Doom3GroupInstance>::Caller(m_instances)
    )
  {
    construct();
  }
  ~Doom3GroupNode()
  {
    destroy();
  }

  void release()
  {
    delete this;
  }
  scene::Node& node()
  {
    return m_node;
  }

  scene::Node& clone() const
  {
    return (new Doom3GroupNode(*this))->node();
  }

  void insert(scene::Node& child)
  {
    m_instances.insert(child);
  }
  void erase(scene::Node& child)
  {
    m_instances.erase(child);
  }

  scene::Instance* create(const scene::Path& path, scene::Instance* parent)
  {
    return new entity::Doom3GroupInstance(path, parent, m_contained);
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
};

void Doom3Group_construct()
{
  CurveEdit::Type::instance().m_controlsShader = GlobalShaderCache().capture("$POINT");
  CurveEdit::Type::instance().m_selectedShader = GlobalShaderCache().capture("$SELPOINT");
}

void Doom3Group_destroy()
{
  GlobalShaderCache().release("$SELPOINT");
  GlobalShaderCache().release("$POINT");
}

scene::Node& New_Doom3Group(IEntityClassPtr eclass)
{
  return (new Doom3GroupNode(eclass))->node();
}
