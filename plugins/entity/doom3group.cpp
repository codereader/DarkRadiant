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

#include "doom3group/Doom3Group.h"

class ControlPointAddBounds {
	AABB& m_bounds;
public:
	ControlPointAddBounds(AABB& bounds) : m_bounds(bounds) {}
	void operator()(const Vector3& point) const {
		m_bounds.includePoint(point);
	}
};

class Doom3GroupInstance :
  public TargetableInstance,
  public TransformModifier,
  public Renderable,
  public SelectionTestable,
  public ComponentSelectionTestable,
  public ComponentEditable,
  public ComponentSnappable
{
  class TypeCasts
  {
    InstanceTypeCastTable m_casts;
  public:
    TypeCasts()
    {
      m_casts = TargetableInstance::StaticTypeCasts::instance().get();
      InstanceContainedCast<Doom3GroupInstance, Bounded>::install(m_casts);
      InstanceStaticCast<Doom3GroupInstance, Renderable>::install(m_casts);
      InstanceStaticCast<Doom3GroupInstance, SelectionTestable>::install(m_casts);
      InstanceStaticCast<Doom3GroupInstance, ComponentSelectionTestable>::install(m_casts);
      InstanceStaticCast<Doom3GroupInstance, ComponentEditable>::install(m_casts);
      InstanceStaticCast<Doom3GroupInstance, ComponentSnappable>::install(m_casts);
      InstanceStaticCast<Doom3GroupInstance, Transformable>::install(m_casts);
      InstanceIdentityCast<Doom3GroupInstance>::install(m_casts);
    }
    InstanceTypeCastTable& get()
    {
      return m_casts;
    }
  };

  entity::Doom3Group& m_contained;
  CurveEdit m_curveNURBS;
  CurveEdit m_curveCatmullRom;
  mutable AABB m_aabb_component;
public:

  typedef LazyStatic<TypeCasts> StaticTypeCasts;


  Bounded& get(NullType<Bounded>)
  {
    return m_contained;
  }

  STRING_CONSTANT(Name, "Doom3GroupInstance");

  Doom3GroupInstance(const scene::Path& path, scene::Instance* parent, entity::Doom3Group& contained) :
    TargetableInstance(path, parent, this, StaticTypeCasts::instance().get(), contained.getEntity(), *this),
    TransformModifier(entity::Doom3Group::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
    m_contained(contained),
    m_curveNURBS(m_contained.m_curveNURBS.m_controlPointsTransformed, SelectionChangedComponentCaller(*this)),
    m_curveCatmullRom(m_contained.m_curveCatmullRom.m_controlPointsTransformed, SelectionChangedComponentCaller(*this))
  {
    m_contained.instanceAttach(Instance::path());
    m_contained.m_curveNURBSChanged = m_contained.m_curveNURBS.connect(CurveEdit::CurveChangedCaller(m_curveNURBS));
    m_contained.m_curveCatmullRomChanged = m_contained.m_curveCatmullRom.connect(CurveEdit::CurveChangedCaller(m_curveCatmullRom));

    StaticRenderableConnectionLines::instance().attach(*this);
  }
  ~Doom3GroupInstance()
  {
    StaticRenderableConnectionLines::instance().detach(*this);

    m_contained.m_curveCatmullRom.disconnect(m_contained.m_curveCatmullRomChanged);
    m_contained.m_curveNURBS.disconnect(m_contained.m_curveNURBSChanged);
    m_contained.instanceDetach(Instance::path());
  }
  void renderSolid(Renderer& renderer, const VolumeTest& volume) const
  {
    m_contained.renderSolid(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());

    m_curveNURBS.renderComponentsSelected(renderer, volume, localToWorld());
    m_curveCatmullRom.renderComponentsSelected(renderer, volume, localToWorld());
  }
  void renderWireframe(Renderer& renderer, const VolumeTest& volume) const
  {
    m_contained.renderWireframe(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());

    m_curveNURBS.renderComponentsSelected(renderer, volume, localToWorld());
    m_curveCatmullRom.renderComponentsSelected(renderer, volume, localToWorld());
  }
  void renderComponents(Renderer& renderer, const VolumeTest& volume) const
  {
    if(GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex)
    {
      m_curveNURBS.renderComponents(renderer, volume, localToWorld());
      m_curveCatmullRom.renderComponents(renderer, volume, localToWorld());
    }
  }

  void testSelect(Selector& selector, SelectionTest& test)
  {
    test.BeginMesh(localToWorld());
    SelectionIntersection best;

    m_contained.testSelect(selector, test, best);

    if(best.valid())
    {
      Selector_add(selector, getSelectable(), best);
    }
  }

  bool isSelectedComponents() const
  {
    return m_curveNURBS.isSelected() || m_curveCatmullRom.isSelected();
  }
  void setSelectedComponents(bool selected, SelectionSystem::EComponentMode mode)
  {
    if(mode == SelectionSystem::eVertex)
    {
      m_curveNURBS.setSelected(selected);
      m_curveCatmullRom.setSelected(selected);
    }
  }
  void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode)
  {
    if(mode == SelectionSystem::eVertex)
    {
      test.BeginMesh(localToWorld());
      m_curveNURBS.testSelect(selector, test);
      m_curveCatmullRom.testSelect(selector, test);
    }
  }

  void transformComponents(const Matrix4& matrix)
  {
    if(m_curveNURBS.isSelected())
    {
      m_curveNURBS.transform(matrix);
    }
    if(m_curveCatmullRom.isSelected())
    {
      m_curveCatmullRom.transform(matrix);
    }
  }

  const AABB& getSelectedComponentsBounds() const
  {
    m_aabb_component = AABB();
    m_curveNURBS.forEachSelected(ControlPointAddBounds(m_aabb_component));
    m_curveCatmullRom.forEachSelected(ControlPointAddBounds(m_aabb_component));
    return m_aabb_component;
  }

  void snapComponents(float snap)
  {
    if(m_curveNURBS.isSelected())
    {
      m_curveNURBS.snapto(snap);
      m_curveNURBS.write(curve_Nurbs, m_contained.getEntity());
    }
    if(m_curveCatmullRom.isSelected())
    {
      m_curveCatmullRom.snapto(snap);
      m_curveCatmullRom.write(curve_CatmullRomSpline, m_contained.getEntity());
    }
  }

  void evaluateTransform()
  {
    if(getType() == TRANSFORM_PRIMITIVE)
    {
      m_contained.translate(getTranslation());
      m_contained.rotate(getRotation());
    }
    else
    {
      transformComponents(calculateTransform());
    }
  }
  void applyTransform()
  {
    m_contained.revertTransform();
    evaluateTransform();
    m_contained.freezeTransform();
  }
  typedef MemberCaller<Doom3GroupInstance, &Doom3GroupInstance::applyTransform> ApplyTransformCaller;

  void selectionChangedComponent(const Selectable& selectable)
  {
    GlobalSelectionSystem().getObserver(SelectionSystem::eComponent)(selectable);
    GlobalSelectionSystem().onComponentSelection(*this, selectable);
  }
  typedef MemberCaller1<Doom3GroupInstance, const Selectable&, &Doom3GroupInstance::selectionChangedComponent> SelectionChangedComponentCaller;
};

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
      m_contained(eclass, m_node, InstanceSet::TransformChangedCaller(m_instances), InstanceSet::BoundsChangedCaller(m_instances), InstanceSetEvaluateTransform<Doom3GroupInstance>::Caller(m_instances))
  {
    construct();
  }
  Doom3GroupNode(const Doom3GroupNode& other) :
    scene::Node::Symbiot(other),
    scene::Instantiable(other),
    scene::Cloneable(other),
    scene::Traversable::Observer(other),
    m_node(this, this, StaticTypeCasts::instance().get()),
    m_contained(other.m_contained, m_node, InstanceSet::TransformChangedCaller(m_instances), InstanceSet::BoundsChangedCaller(m_instances), InstanceSetEvaluateTransform<Doom3GroupInstance>::Caller(m_instances))
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
    return new Doom3GroupInstance(path, parent, m_contained);
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
