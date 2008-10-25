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

#if !defined(INCLUDED_BRUSHNODE_H)
#define INCLUDED_BRUSHNODE_H

#include "TexDef.h"
#include "ibrush.h"
#include "Brush.h"
#include "BrushTokenImporter.h"
#include "BrushTokenExporter.h"
#include "nameable.h"
#include "selectionlib.h"
#include "FaceInstance.h"
#include "EdgeInstance.h"
#include "VertexInstance.h"
#include "BrushClipPlane.h"

class BrushInstanceVisitor {
public:
	virtual void visit(FaceInstance& face) const = 0;
};

class BrushNode :
	public scene::Node,
	public scene::Cloneable,
	public Nameable,
	public Snappable,
	public TransformNode,
	public BrushDoom3,
	public BrushTokenImporter, // implements MapImporter
	public BrushTokenExporter, // implements MapExporter
	public IBrushNode,
	public Selectable,
	public BrushObserver,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public LightCullable,
	public Renderable,
	public Cullable,
	public Bounded,
	public TransformModifier	// inherits from Transformable
{
	// The actual contained brush (NO reference)
	Brush m_brush;

	// Implements the Selectable interface
	ObservedSelectable _selectable;

	FaceInstances m_faceInstances;

	typedef std::vector<EdgeInstance> EdgeInstances;
	EdgeInstances m_edgeInstances;
	typedef std::vector<brush::VertexInstance> VertexInstances;
	VertexInstances m_vertexInstances;

	mutable RenderableWireframe m_render_wireframe;
	mutable RenderablePointVector m_render_selected;
	mutable AABB m_aabb_component;
	mutable Array<PointVertex> m_faceCentroidPointsCulled;
	RenderablePointArray m_render_faces_wireframe;
	mutable bool m_viewChanged; // requires re-evaluation of view-dependent cached data

	BrushClipPlane m_clipPlane;

	static ShaderPtr m_state_selpoint;

	const LightList* m_lightList;
	
public:
	// Constructor
	BrushNode();
	
	// Copy Constructor
	BrushNode(const BrushNode& other);

	virtual ~BrushNode();

	// IBrushNode implementtation
	virtual Brush& getBrush();
	
	std::string name() const {
		return "Brush";
	}

	void lightsChanged();
	typedef MemberCaller<BrushNode, &BrushNode::lightsChanged> LightsChangedCaller;

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Cullable implementation
	virtual VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;
	
	// TransformNode implementation
	virtual const Matrix4& localToParent() const;

	// Selectable implementation
	virtual bool isSelected() const;
	virtual void setSelected(bool select);
	virtual void invertSelected();

	// SelectionTestable implementation
	virtual void testSelect(Selector& selector, SelectionTest& test);

	// ComponentSelectionTestable
	bool isSelectedComponents() const;
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	// override scene::Inode::onRemoveFromScene to deselect the child components
	virtual void onRemoveFromScene();

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const;

	// The callback for the ObservedSelectable
	void selectedChanged(const Selectable& selectable);
	typedef MemberCaller1<BrushNode, const Selectable&, &BrushNode::selectedChanged> SelectedChangedCaller;

	void selectedChangedComponent(const Selectable& selectable);
	typedef MemberCaller1<BrushNode, const Selectable&, &BrushNode::selectedChangedComponent> SelectedChangedComponentCaller;

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

	// Snappable implementation
	virtual void snapto(float snap);

	// ComponentSnappable implementation
	void snapComponents(float snap);

	// BrushDoom3 implementation
	virtual void translateDoom3Brush(const Vector3& translation);

	// Allocates a new node on the heap (via copy construction)
	scene::INodePtr clone() const;

	static void constructStatic();
	static void destroyStatic();
	
	// scene::Instantiable implementation
	virtual void instantiate(const scene::Path& path);
	virtual void uninstantiate(const scene::Path& path);
	
	// BrushObserver implementation
	void clear();
	void reserve(std::size_t size);
	void push_back(Face& face);
	void pop_back();
	void erase(std::size_t index);
	void connectivityChanged();
	void edge_clear();
	void edge_push_back(SelectableEdge& edge);
	void vertex_clear();
	void vertex_push_back(SelectableVertex& vertex);
	void DEBUG_verify();

	// LightCullable implementation
	bool testLight(const RendererLight& light) const;
	void insertLight(const RendererLight& light);
	void clearLights();

	// Renderable implementation
	void renderComponents(Renderer& renderer, const VolumeTest& volume) const;
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const;
	void viewChanged() const;

	// Gets called by the TransformModifier
	void applyTransform();
	typedef MemberCaller<BrushNode, &BrushNode::applyTransform> ApplyTransformCaller;

	void evaluateTransform();
	typedef MemberCaller<BrushNode, &BrushNode::evaluateTransform> EvaluateTransformCaller;

	void setClipPlane(const Plane3& plane);

	const BrushInstanceVisitor& forEachFaceInstance(const BrushInstanceVisitor& visitor);

private:
	void transformComponents(const Matrix4& matrix);

	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void update_selected() const;
	void renderComponentsSelected(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void renderClipPlane(Renderer& renderer, const VolumeTest& volume) const;
	void evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const;
	
}; // class BrushNode
typedef boost::shared_ptr<BrushNode> BrushNodePtr;

#endif
