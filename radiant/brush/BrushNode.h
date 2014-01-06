#pragma once

#include "TexDef.h"
#include "ibrush.h"
#include "itraceable.h"
#include "Brush.h"
#include "SelectableNode.h"
#include "FaceInstance.h"
#include "EdgeInstance.h"
#include "VertexInstance.h"
#include "BrushClipPlane.h"
#include "transformlib.h"
#include "scene/Node.h"

class BrushNode :
	public scene::SelectableNode,
	public scene::Cloneable,
	public Snappable,
	public IdentityTransform,
	public Translatable,
	public IBrushNode,
	public BrushObserver,
	public SelectionTestable,
	public ComponentSelectionTestable,
	public ComponentEditable,
	public ComponentSnappable,
	public PlaneSelectable,
	public LitObject,
	public Transformable,
	public ITraceable
{
	LightList* m_lightList;

	// The actual contained brush (NO reference)
	Brush m_brush;

	FaceInstances m_faceInstances;

	typedef std::vector<EdgeInstance> EdgeInstances;
	EdgeInstances m_edgeInstances;
	typedef std::vector<brush::VertexInstance> VertexInstances;
	VertexInstances m_vertexInstances;

	mutable RenderableWireframe m_render_wireframe;

    // Renderable array of vertex and edge points
	mutable RenderablePointVector _selectedPoints;

	mutable AABB m_aabb_component;
	mutable RenderablePointVector _faceCentroidPointsCulled;
	mutable bool m_viewChanged; // requires re-evaluation of view-dependent cached data

	BrushClipPlane m_clipPlane;

	ShaderPtr m_state_selpoint;

	// TRUE if any of the FaceInstance's component selection got changed or transformed
	mutable bool _renderableComponentsNeedUpdate;

public:
	// Constructor
	BrushNode();

	// Copy Constructor
	BrushNode(const BrushNode& other);

	virtual ~BrushNode();

	// IBrushNode implementtation
	virtual Brush& getBrush();
	virtual IBrush& getIBrush();

	std::string name() const {
		return "Brush";
	}

	Type getNodeType() const;

	void lightsChanged();

	// Bounded implementation
	virtual const AABB& localAABB() const;

	// Override ObservedSelectable implementation
	virtual void invertSelected();

	// SelectionTestable implementation
	virtual void testSelect(Selector& selector, SelectionTest& test);

	// ComponentSelectionTestable
	bool isSelectedComponents() const;
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode);
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode);

	// override scene::Inode::onRemoveFromScene to deselect the child components
	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const;

	void selectedChangedComponent(const Selectable& selectable);

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback);
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes);

	// Snappable implementation
	virtual void snapto(float snap);

	// ComponentSnappable implementation
	void snapComponents(float snap);

	// Translatable implementation
	virtual void translate(const Vector3& translation);

	// Allocates a new node on the heap (via copy construction)
	scene::INodePtr clone() const;

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

	// LitObject implementation
	bool intersectsLight(const RendererLight& light) const;
	void insertLight(const RendererLight& light);
	void clearLights();

	// Renderable implementation
	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
	void setRenderSystem(const RenderSystemPtr& renderSystem);

	void viewChanged() const;
	bool isHighlighted() const;

	void evaluateTransform();

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection);

	// Update call, issued by the FilterSystem on potential shader visibility changes
	void updateFaceVisibility();

	void setClipPlane(const Plane3& plane);

	void forEachFaceInstance(const std::function<void(FaceInstance&)>& functor);

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged();

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
	void _applyTransformation();

private:
	void transformComponents(const Matrix4& matrix);

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;

	void update_selected() const;
	void renderSelectedPoints(RenderableCollector& collector,
                              const VolumeTest& volume,
                              const Matrix4& localToWorld) const;

	void renderClipPlane(RenderableCollector& collector, const VolumeTest& volume) const;
	void evaluateViewDependent(const VolumeTest& volume, const Matrix4& localToWorld) const;

}; // class BrushNode
typedef boost::shared_ptr<BrushNode> BrushNodePtr;
