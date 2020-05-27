#pragma once

#include "TexDef.h"
#include "ibrush.h"
#include "itraceable.h"
#include "iscenegraph.h"

#include "Brush.h"
#include "scene/SelectableNode.h"
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

    // For pivoted rotations, we need a copy of this lying around
    Vector3 _untransformedOrigin;
    // If true, the _untransformedOrigin member needs an update
    bool _untransformedOriginChanged;

public:
	// Constructor
	BrushNode();

	// Copy Constructor
	BrushNode(const BrushNode& other);

	virtual ~BrushNode();

	// IBrushNode implementtation
	virtual Brush& getBrush() override;
	virtual IBrush& getIBrush() override;

	std::string name() const  override
    {
		return "Brush";
	}

	Type getNodeType() const override;

	void lightsChanged();

	// Bounded implementation
	virtual const AABB& localAABB() const override;

	// SelectionTestable implementation
	virtual void testSelect(Selector& selector, SelectionTest& test) override;

	// ComponentSelectionTestable
	bool isSelectedComponents() const override;
	void setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) override;
	void invertSelectedComponents(SelectionSystem::EComponentMode mode) override;
	void testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) override;

	// override scene::Inode::onRemoveFromScene to deselect the child components
    virtual void onInsertIntoScene(scene::IMapRootNode& root) override;
    virtual void onRemoveFromScene(scene::IMapRootNode& root) override;

	// ComponentEditable implementation
	const AABB& getSelectedComponentsBounds() const override;

	void selectedChangedComponent(const ISelectable& selectable);

	// PlaneSelectable implementation
	void selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) override;
	void selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) override;

	// Snappable implementation
	virtual void snapto(float snap) override;

	// ComponentSnappable implementation
	void snapComponents(float snap) override;

	// Translatable implementation
	virtual void translate(const Vector3& translation) override;

	// Allocates a new node on the heap (via copy construction)
	scene::INodePtr clone() const override;

	// BrushObserver implementation
	void clear() override;
	void reserve(std::size_t size) override;
	void push_back(Face& face) override;
	void pop_back() override;
	void erase(std::size_t index) override;
	void connectivityChanged() override;
	void edge_clear() override;
	void edge_push_back(SelectableEdge& edge) override;
	void vertex_clear() override;
	void vertex_push_back(SelectableVertex& vertex) override;
	void DEBUG_verify() override;

	// LitObject implementation
	bool intersectsLight(const RendererLight& light) const override;
	void insertLight(const RendererLight& light) override;
	void clearLights() override;

	// Renderable implementation
	void renderComponents(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;
	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	void viewChanged() const override;
	std::size_t getHighlightFlags() override;

	void evaluateTransform();

	// Traceable implementation
	bool getIntersection(const Ray& ray, Vector3& intersection) override;

	// Update call, issued by the FilterSystem on potential shader visibility changes
	void updateFaceVisibility();

	void setClipPlane(const Plane3& plane);

	void forEachFaceInstance(const std::function<void(FaceInstance&)>& functor);

    // Returns the center of the untransformed world AABB
    const Vector3& getUntransformedOrigin() override;

protected:
	// Gets called by the Transformable implementation whenever
	// scale, rotation or translation is changed.
	void _onTransformationChanged() override;

	// Called by the Transformable implementation before freezing
	// or when reverting transformations.
    void _applyTransformation() override;

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
typedef std::shared_ptr<BrushNode> BrushNodePtr;
